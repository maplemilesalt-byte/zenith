#include "window.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cstring>
#include <stdexcept>

struct XenithWindow::Impl {
    Display* display = nullptr;
    ::Window window = 0;
    GC gc = nullptr;
    XImage* image = nullptr;
    int width = 0;
    int height = 0;
    bool open = false;
    bool fileMenu = false;
    bool importRequested = false;
};

XenithWindow::XenithWindow(int width, int height, const char* title) : impl_(new Impl{}) {
    impl_->width = width; impl_->height = height;
    impl_->display = XOpenDisplay(nullptr);
    if (!impl_->display) { delete impl_; impl_=nullptr; throw std::runtime_error("Could not open X11 display"); }
    const int screen=DefaultScreen(impl_->display);
    impl_->window=XCreateSimpleWindow(impl_->display,RootWindow(impl_->display,screen),0,0,
        static_cast<unsigned>(width),static_cast<unsigned>(height),1,BlackPixel(impl_->display,screen),BlackPixel(impl_->display,screen));
    XStoreName(impl_->display,impl_->window,title);
    XSelectInput(impl_->display,impl_->window,ExposureMask|KeyPressMask|ButtonPressMask|StructureNotifyMask);
    XMapWindow(impl_->display,impl_->window); impl_->gc=XCreateGC(impl_->display,impl_->window,0,nullptr); impl_->open=true;
}
XenithWindow::~XenithWindow(){if(!impl_)return;if(impl_->image){impl_->image->data=nullptr;XDestroyImage(impl_->image);}if(impl_->gc)XFreeGC(impl_->display,impl_->gc);if(impl_->window)XDestroyWindow(impl_->display,impl_->window);if(impl_->display)XCloseDisplay(impl_->display);delete impl_;}
bool XenithWindow::isOpen()const{return impl_&&impl_->open;}
void XenithWindow::pollEvents(){if(!impl_)return;while(XPending(impl_->display)){XEvent e{};XNextEvent(impl_->display,&e);if(e.type==DestroyNotify||e.type==KeyPress)impl_->open=false;else if(e.type==ButtonPress&&e.xbutton.button==Button1){if(e.xbutton.y<28&&e.xbutton.x<65)impl_->fileMenu=!impl_->fileMenu;else if(impl_->fileMenu&&e.xbutton.x<230&&e.xbutton.y>=28&&e.xbutton.y<58){impl_->importRequested=true;impl_->fileMenu=false;}else impl_->fileMenu=false;}}}
void XenithWindow::present(const std::uint32_t* pixels,int width,int height){if(!impl_||!impl_->open||!pixels||width!=impl_->width||height!=impl_->height)return;if(!impl_->image){const int depth=DefaultDepth(impl_->display,DefaultScreen(impl_->display));auto* data=new char[static_cast<std::size_t>(width)*height*4];impl_->image=XCreateImage(impl_->display,DefaultVisual(impl_->display,DefaultScreen(impl_->display)),depth,ZPixmap,0,data,width,height,32,0);if(!impl_->image){delete[]data;throw std::runtime_error("Could not create XImage");}}std::memcpy(impl_->image->data,pixels,static_cast<std::size_t>(width)*height*4);XPutImage(impl_->display,impl_->window,impl_->gc,impl_->image,0,0,0,0,static_cast<unsigned>(width),static_cast<unsigned>(height));
    XSetForeground(impl_->display,impl_->gc,0x202020);XFillRectangle(impl_->display,impl_->window,impl_->gc,0,0,impl_->width,28);XSetForeground(impl_->display,impl_->gc,0xffffff);XDrawString(impl_->display,impl_->window,impl_->gc,8,19,"File",4);XDrawString(impl_->display,impl_->window,impl_->gc,70,19,"Emulation",9);XDrawString(impl_->display,impl_->window,impl_->gc,145,19,"View",4);XDrawString(impl_->display,impl_->window,impl_->gc,190,19,"Help",4);
    if(impl_->fileMenu){XSetForeground(impl_->display,impl_->gc,0x303030);XFillRectangle(impl_->display,impl_->window,impl_->gc,0,28,230,30);XSetForeground(impl_->display,impl_->gc,0xffffff);XDrawString(impl_->display,impl_->window,impl_->gc,10,48,"Import ELF...",12);XDrawString(impl_->display,impl_->window,impl_->gc,120,48,"APPX (future)",13);}XFlush(impl_->display);}
bool XenithWindow::menuImportRequested(){if(!impl_)return false;bool r=impl_->importRequested;impl_->importRequested=false;return r;}
bool XenithWindow::menuBarHit(int x,int y)const{return impl_&&y<28&&x<65;}
bool XenithWindow::fileMenuOpen()const{return impl_&&impl_->fileMenu;}
void XenithWindow::setTitle(const std::string& title){if(impl_&&impl_->display&&impl_->window)XStoreName(impl_->display,impl_->window,title.c_str());}
