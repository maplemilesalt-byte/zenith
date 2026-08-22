#include "wayland_window.h"
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <cstring>
#include <stdexcept>

struct WaylandWindow::Impl {
    wl_display* display=nullptr; wl_registry* registry=nullptr; wl_compositor* compositor=nullptr; wl_shm* shm=nullptr; xdg_wm_base* wm=nullptr;
    wl_surface* surface=nullptr; xdg_surface* xsurface=nullptr; xdg_toplevel* toplevel=nullptr; wl_buffer* buffer=nullptr;
    void* mapped=nullptr; int fd=-1; int width=0,height=0; bool open=false,busy=false,fileMenu=false,importRequested=false;
};

static void wm_ping(void*,xdg_wm_base* wm,std::uint32_t serial){xdg_wm_base_pong(wm,serial);}
static void surface_configure(void*,xdg_surface* surface,std::uint32_t serial){xdg_surface_ack_configure(surface,serial);}
static void toplevel_configure(void*,xdg_toplevel*,std::int32_t,std::int32_t,wl_array*){}
static void toplevel_close(void* data,xdg_toplevel*){static_cast<WaylandWindow::Impl*>(data)->open=false;}
static const wl_registry_listener registry_listener={
    [](void* data,wl_registry* registry,std::uint32_t name,const char* interface,std::uint32_t version){
        auto* p=static_cast<WaylandWindow::Impl*>(data);
        if(std::strcmp(interface,wl_compositor_interface.name)==0)p->compositor=static_cast<wl_compositor*>(wl_registry_bind(registry,name,&wl_compositor_interface,4));
        else if(std::strcmp(interface,wl_shm_interface.name)==0)p->shm=static_cast<wl_shm*>(wl_registry_bind(registry,name,&wl_shm_interface,1));
        else if(std::strcmp(interface,xdg_wm_base_interface.name)==0)p->wm=static_cast<xdg_wm_base*>(wl_registry_bind(registry,name,&xdg_wm_base_interface,1));
        (void)version;
    },
    [](void*,wl_registry*,std::uint32_t){}
};
static const xdg_wm_base_listener wm_listener={wm_ping};
static const xdg_surface_listener surface_listener={surface_configure};
static const xdg_toplevel_listener toplevel_listener={toplevel_configure,toplevel_close};
static void buffer_release(void* data,wl_buffer*){static_cast<WaylandWindow::Impl*>(data)->busy=false;}
static const wl_buffer_listener buffer_listener={buffer_release};

static int make_shm(std::size_t size){
    char name[]="/zenith-shm-XXXXXX"; int fd=mkstemp(name); if(fd<0)return -1; unlink(name);
    if(ftruncate(fd,(off_t)size)!=0){close(fd);return -1;} return fd;
}

WaylandWindow::WaylandWindow(int width,int height,const char* title,InputState& input):impl_(new Impl{}),input_(input){
    impl_->width=width; impl_->height=height;
    impl_->display=wl_display_connect(nullptr);
    if(!impl_->display){delete impl_;impl_=nullptr;throw std::runtime_error("Could not connect to Wayland display");}
    impl_->registry=wl_display_get_registry(impl_->display); wl_registry_add_listener(impl_->registry,&registry_listener,impl_); wl_display_roundtrip(impl_->display);
    if(!impl_->compositor||!impl_->shm||!impl_->wm)throw std::runtime_error("Wayland compositor lacks required globals");
    xdg_wm_base_add_listener(impl_->wm,&wm_listener,impl_);
    impl_->surface=wl_compositor_create_surface(impl_->compositor);
    impl_->xsurface=xdg_wm_base_get_xdg_surface(impl_->wm,impl_->surface); xdg_surface_add_listener(impl_->xsurface,&surface_listener,impl_);
    impl_->toplevel=xdg_surface_get_toplevel(impl_->xsurface); xdg_toplevel_add_listener(impl_->toplevel,&toplevel_listener,impl_); xdg_toplevel_set_title(impl_->toplevel,title);
    wl_surface_commit(impl_->surface); wl_display_roundtrip(impl_->display); impl_->open=true;
}
WaylandWindow::~WaylandWindow(){
    if(!impl_)return;
    if(impl_->buffer)wl_buffer_destroy(impl_->buffer); if(impl_->mapped)munmap(impl_->mapped,(std::size_t)impl_->width*impl_->height*4); if(impl_->fd>=0)close(impl_->fd);
    if(impl_->toplevel)xdg_toplevel_destroy(impl_->toplevel); if(impl_->xsurface)xdg_surface_destroy(impl_->xsurface); if(impl_->surface)wl_surface_destroy(impl_->surface);
    if(impl_->wm)xdg_wm_base_destroy(impl_->wm); if(impl_->shm)wl_shm_destroy(impl_->shm); if(impl_->compositor)wl_compositor_destroy(impl_->compositor); if(impl_->registry)wl_registry_destroy(impl_->registry); if(impl_->display)wl_display_disconnect(impl_->display); delete impl_;
}
bool WaylandWindow::isOpen()const{return impl_&&impl_->open;}
void WaylandWindow::pollEvents(){
    if(!impl_||!impl_->display)return; wl_display_dispatch_pending(impl_->display); wl_display_flush(impl_->display);
    pollfd pfd{wl_display_get_fd(impl_->display),POLLIN,0}; if(poll(&pfd,1,0)>0)wl_display_dispatch(impl_->display);
}
void WaylandWindow::present(const std::uint32_t* pixels,int width,int height){
    if(!impl_||!impl_->open||!pixels||width!=impl_->width||height!=impl_->height||impl_->busy)return;
    const std::size_t bytes=(std::size_t)width*height*4;
    if(!impl_->buffer){
        impl_->fd=make_shm(bytes); if(impl_->fd<0)throw std::runtime_error("Could not create Wayland SHM buffer");
        impl_->mapped=mmap(nullptr,bytes,PROT_READ|PROT_WRITE,MAP_SHARED,impl_->fd,0); if(impl_->mapped==MAP_FAILED)throw std::runtime_error("Could not map Wayland SHM buffer");
        wl_shm_pool* pool=wl_shm_create_pool(impl_->shm,impl_->fd,(int)bytes); impl_->buffer=wl_shm_pool_create_buffer(pool,0,width,height,width*4,WL_SHM_FORMAT_XRGB8888); wl_shm_pool_destroy(pool); wl_buffer_add_listener(impl_->buffer,&buffer_listener,impl_);
    }
    std::memcpy(impl_->mapped,pixels,bytes);
    auto* p=static_cast<std::uint32_t*>(impl_->mapped); for(int y=0;y<28&&y<height;++y)for(int x=0;x<width;++x)p[(std::size_t)y*width+x]=0x00202020u;
    wl_surface_attach(impl_->surface,impl_->buffer,0,0); wl_surface_damage_buffer(impl_->surface,0,0,width,height); wl_surface_commit(impl_->surface); impl_->busy=true; wl_display_flush(impl_->display);
}
bool WaylandWindow::menuImportRequested(){if(!impl_)return false;bool r=impl_->importRequested;impl_->importRequested=false;return r;}bool WaylandWindow::menuBarHit(int x,int y)const{return impl_&&y<28&&x<65;}bool WaylandWindow::fileMenuOpen()const{return impl_&&impl_->fileMenu;}void WaylandWindow::setTitle(const std::string& title){if(impl_&&impl_->toplevel)xdg_toplevel_set_title(impl_->toplevel,title.c_str());}
