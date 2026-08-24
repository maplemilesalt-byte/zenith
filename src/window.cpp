#include "window.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <linux/input.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>

struct XenithWindow::Impl {
    struct Gamepad {
        int fd=-1;
        input_absinfo abs[ABS_MAX+1]{};
        bool hasAbsX=false,hasAbsY=false,hasGamepadButtons=false;
    };
    Display* display=nullptr; ::Window window=0; GC gc=nullptr; XImage* image=nullptr;
    int width=0,height=0; bool open=false,fileMenu=false,importRequested=false;
    Gamepad gamepad{};
};

static bool testBit(const unsigned long* bits,int bit){return (bits[bit/(sizeof(unsigned long)*8)]>>(bit%(sizeof(unsigned long)*8)))&1UL;}

static void openGamepad(XenithWindow::Impl& impl){
    for(int index=0;index<32;++index){
        const std::string path="/dev/input/event"+std::to_string(index);
        const int fd=open(path.c_str(),O_RDONLY|O_NONBLOCK|O_CLOEXEC);
        if(fd<0)continue;
        unsigned long evBits[(EV_MAX/(sizeof(unsigned long)*8))+1]{};
        unsigned long keyBits[(KEY_MAX/(sizeof(unsigned long)*8))+1]{};
        unsigned long absBits[(ABS_MAX/(sizeof(unsigned long)*8))+1]{};
        if(ioctl(fd,EVIOCGBIT(0,sizeof(evBits)),evBits)<0||ioctl(fd,EVIOCGBIT(EV_KEY,sizeof(keyBits)),keyBits)<0||ioctl(fd,EVIOCGBIT(EV_ABS,sizeof(absBits)),absBits)<0){close(fd);continue;}
        const bool hasAxes=testBit(absBits,ABS_X)&&testBit(absBits,ABS_Y);
        const bool hasButtons=testBit(keyBits,BTN_SOUTH)||testBit(keyBits,BTN_GAMEPAD);
        if(!testBit(evBits,EV_ABS)||!testBit(evBits,EV_KEY)||!hasAxes||!hasButtons){close(fd);continue;}
        impl.gamepad.fd=fd;impl.gamepad.hasAbsX=true;impl.gamepad.hasAbsY=true;impl.gamepad.hasGamepadButtons=true;
        for(int axis=0;axis<=ABS_MAX;++axis)if(testBit(absBits,axis))ioctl(fd,EVIOCGABS(axis),&impl.gamepad.abs[axis]);
        char name[256]{};ioctl(fd,EVIOCGNAME(sizeof(name)),name);
        std::string title="Zenith - Xbox Series X|S Emulator";
        if(name[0])title+=" - "+std::string(name);
        if(impl.display&&impl.window)XStoreName(impl.display,impl.window,title.c_str());
        return;
    }
}

static float normalizeAxis(const input_absinfo& info,int value){
    const float min=static_cast<float>(info.minimum),max=static_cast<float>(info.maximum);
    if(max<=min)return 0.0f;
    const float normalized=(static_cast<float>(value)-min)/(max-min);
    return normalized*2.0f-1.0f;
}

static float normalizeTrigger(const input_absinfo& info,int value){
    const float min=static_cast<float>(info.minimum),max=static_cast<float>(info.maximum);
    if(max<=min)return 0.0f;
    const float normalized=(static_cast<float>(value)-min)/(max-min);
    return normalized*2.0f-1.0f;
}

static void pollGamepad(XenithWindow::Impl& impl,InputState& input){
    if(impl.gamepad.fd<0)return;
    input_event event{};
    while(read(impl.gamepad.fd,&event,sizeof(event))==static_cast<ssize_t>(sizeof(event))){
        if(event.type==EV_KEY){
            const bool pressed=event.value!=0;
            switch(event.code){
                case BTN_SOUTH:input.setGamepadButton(ZenithGamepad::A,pressed);break;
                case BTN_EAST:input.setGamepadButton(ZenithGamepad::B,pressed);break;
                case BTN_WEST:input.setGamepadButton(ZenithGamepad::X,pressed);break;
                case BTN_NORTH:input.setGamepadButton(ZenithGamepad::Y,pressed);break;
                case BTN_TL:input.setGamepadButton(ZenithGamepad::LB,pressed);break;
                case BTN_TR:input.setGamepadButton(ZenithGamepad::RB,pressed);break;
                case BTN_THUMBL:input.setGamepadButton(ZenithGamepad::LS,pressed);break;
                case BTN_THUMBR:input.setGamepadButton(ZenithGamepad::RS,pressed);break;
                case BTN_SELECT:input.setGamepadButton(ZenithGamepad::View,pressed);break;
                case BTN_START:input.setGamepadButton(ZenithGamepad::Menu,pressed);break;
                case BTN_DPAD_UP:input.setGamepadButton(ZenithGamepad::DpadUp,pressed);break;
                case BTN_DPAD_DOWN:input.setGamepadButton(ZenithGamepad::DpadDown,pressed);break;
                case BTN_DPAD_LEFT:input.setGamepadButton(ZenithGamepad::DpadLeft,pressed);break;
                case BTN_DPAD_RIGHT:input.setGamepadButton(ZenithGamepad::DpadRight,pressed);break;
                default:break;
            }
        }else if(event.type==EV_ABS){
            switch(event.code){
                case ABS_X:input.setGamepadAxis(ZenithGamepad::LeftX,normalizeAxis(impl.gamepad.abs[ABS_X],event.value));break;
                case ABS_Y:input.setGamepadAxis(ZenithGamepad::LeftY,normalizeAxis(impl.gamepad.abs[ABS_Y],event.value));break;
                case ABS_RX:input.setGamepadAxis(ZenithGamepad::RightX,normalizeAxis(impl.gamepad.abs[ABS_RX],event.value));break;
                case ABS_RY:input.setGamepadAxis(ZenithGamepad::RightY,normalizeAxis(impl.gamepad.abs[ABS_RY],event.value));break;
                case ABS_Z:input.setGamepadAxis(ZenithGamepad::LeftTrigger,normalizeTrigger(impl.gamepad.abs[ABS_Z],event.value));break;
                case ABS_RZ:input.setGamepadAxis(ZenithGamepad::RightTrigger,normalizeTrigger(impl.gamepad.abs[ABS_RZ],event.value));break;
                case ABS_HAT0X:
                    input.setGamepadButton(ZenithGamepad::DpadLeft,event.value<0);input.setGamepadButton(ZenithGamepad::DpadRight,event.value>0);break;
                case ABS_HAT0Y:
                    input.setGamepadButton(ZenithGamepad::DpadUp,event.value<0);input.setGamepadButton(ZenithGamepad::DpadDown,event.value>0);break;
                default:break;
            }
        }
    }
    if(errno!=EAGAIN&&errno!=EWOULDBLOCK){close(impl.gamepad.fd);impl.gamepad.fd=-1;}
}

XenithWindow::XenithWindow(int width,int height,const char* title):impl_(new Impl{}){
    impl_->width=width;impl_->height=height;input_.reset();
    impl_->display=XOpenDisplay(nullptr);
    if(!impl_->display){delete impl_;impl_=nullptr;throw std::runtime_error("Could not open X11 display");}
    const int screen=DefaultScreen(impl_->display);
    impl_->window=XCreateSimpleWindow(impl_->display,RootWindow(impl_->display,screen),0,0,(unsigned)width,(unsigned)height,1,BlackPixel(impl_->display,screen),BlackPixel(impl_->display,screen));
    XStoreName(impl_->display,impl_->window,title);XSelectInput(impl_->display,impl_->window,ExposureMask|KeyPressMask|KeyReleaseMask|ButtonPressMask|StructureNotifyMask);XMapWindow(impl_->display,impl_->window);impl_->gc=XCreateGC(impl_->display,impl_->window,0,nullptr);impl_->open=true;
    openGamepad(*impl_);
}
XenithWindow::~XenithWindow(){if(!impl_)return;if(impl_->gamepad.fd>=0)close(impl_->gamepad.fd);if(impl_->image){impl_->image->data=nullptr;XDestroyImage(impl_->image);}if(impl_->gc)XFreeGC(impl_->display,impl_->gc);if(impl_->window)XDestroyWindow(impl_->display,impl_->window);if(impl_->display)XCloseDisplay(impl_->display);delete impl_;}
bool XenithWindow::isOpen()const{return impl_&&impl_->open;}
void XenithWindow::pollEvents(){
    if(!impl_)return;
    while(XPending(impl_->display)){XEvent e{};XNextEvent(impl_->display,&e);if(e.type==DestroyNotify){impl_->open=false;continue;}if(e.type==KeyPress||e.type==KeyRelease){const bool pressed=e.type==KeyPress;const KeySym key=XLookupKeysym(&e.xkey,0);std::uint32_t mapped=0;switch(key){case XK_w:case XK_W:mapped=ZenithKey::W;break;case XK_a:case XK_A:mapped=ZenithKey::A;break;case XK_s:case XK_S:mapped=ZenithKey::S;break;case XK_d:case XK_D:mapped=ZenithKey::D;break;case XK_Up:mapped=ZenithKey::Up;break;case XK_Down:mapped=ZenithKey::Down;break;case XK_Left:mapped=ZenithKey::Left;break;case XK_Right:mapped=ZenithKey::Right;break;case XK_space:mapped=ZenithKey::Space;break;case XK_Control_L:case XK_Control_R:mapped=ZenithKey::Ctrl;break;case XK_e:case XK_E:mapped=ZenithKey::E;break;case XK_q:case XK_Q:mapped=ZenithKey::Q;break;case XK_z:case XK_Z:mapped=ZenithKey::Z;break;case XK_c:case XK_C:mapped=ZenithKey::C;break;case XK_r:case XK_R:mapped=ZenithKey::R;break;case XK_f:case XK_F:mapped=ZenithKey::F;break;case XK_v:case XK_V:mapped=ZenithKey::V;break;case XK_g:case XK_G:mapped=ZenithKey::G;break;case XK_Tab:mapped=ZenithKey::Tab;break;case XK_Return:mapped=ZenithKey::Enter;break;default:break;}if(mapped)input_.setKey(mapped,pressed);continue;}if(e.type==ButtonPress&&e.xbutton.button==Button1){if(e.xbutton.y<28&&e.xbutton.x<65)impl_->fileMenu=!impl_->fileMenu;else if(impl_->fileMenu&&e.xbutton.x<230&&e.xbutton.y>=28&&e.xbutton.y<58){impl_->importRequested=true;impl_->fileMenu=false;}else impl_->fileMenu=false;}}
    pollGamepad(*impl_,input_);
}
void XenithWindow::present(const std::uint32_t* pixels,int width,int height){if(!impl_||!impl_->open||!pixels||width!=impl_->width||height!=impl_->height)return;if(!impl_->image){const int depth=DefaultDepth(impl_->display,DefaultScreen(impl_->display));auto* data=new char[(std::size_t)width*height*4];impl_->image=XCreateImage(impl_->display,DefaultVisual(impl_->display,DefaultScreen(impl_->display)),depth,ZPixmap,0,data,width,height,32,0);if(!impl_->image){delete[]data;throw std::runtime_error("Could not create XImage");}}std::memcpy(impl_->image->data,pixels,(std::size_t)width*height*4);XPutImage(impl_->display,impl_->window,impl_->gc,impl_->image,0,0,0,0,(unsigned)width,(unsigned)height);XSetForeground(impl_->display,impl_->gc,0x202020);XFillRectangle(impl_->display,impl_->window,impl_->gc,0,0,impl_->width,28);XSetForeground(impl_->display,impl_->gc,0xffffff);XDrawString(impl_->display,impl_->window,impl_->gc,8,19,"File",4);XDrawString(impl_->display,impl_->window,impl_->gc,70,19,"Emulation",9);XDrawString(impl_->display,impl_->window,impl_->gc,145,19,"View",4);XDrawString(impl_->display,impl_->window,impl_->gc,190,19,"Help",4);if(impl_->fileMenu){XSetForeground(impl_->display,impl_->gc,0x303030);XFillRectangle(impl_->display,impl_->window,impl_->gc,0,28,230,30);XSetForeground(impl_->display,impl_->gc,0xffffff);XDrawString(impl_->display,impl_->window,impl_->gc,10,48,"Import ELF...",12);XDrawString(impl_->display,impl_->window,impl_->gc,120,48,"APPX (future)",13);}XFlush(impl_->display);}
bool XenithWindow::menuImportRequested(){if(!impl_)return false;bool r=impl_->importRequested;impl_->importRequested=false;return r;}bool XenithWindow::menuBarHit(int x,int y)const{return impl_&&y<28&&x<65;}bool XenithWindow::fileMenuOpen()const{return impl_&&impl_->fileMenu;}void XenithWindow::setTitle(const std::string& title){if(impl_&&impl_->display&&impl_->window)XStoreName(impl_->display,impl_->window,title.c_str());}const InputState& XenithWindow::input()const{return input_;}
