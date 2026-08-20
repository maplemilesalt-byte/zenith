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
};

XenithWindow::XenithWindow(int width, int height, const char* title)
    : impl_(new Impl{}) {
    impl_->width = width;
    impl_->height = height;
    impl_->display = XOpenDisplay(nullptr);
    if (!impl_->display) {
        delete impl_;
        impl_ = nullptr;
        throw std::runtime_error("Could not open X11 display");
    }

    const int screen = DefaultScreen(impl_->display);
    impl_->window = XCreateSimpleWindow(
        impl_->display, RootWindow(impl_->display, screen), 0, 0,
        static_cast<unsigned>(width), static_cast<unsigned>(height), 1,
        BlackPixel(impl_->display, screen), BlackPixel(impl_->display, screen));

    XStoreName(impl_->display, impl_->window, title);
    XSelectInput(impl_->display, impl_->window,
                 ExposureMask | KeyPressMask | StructureNotifyMask);
    XMapWindow(impl_->display, impl_->window);
    impl_->gc = XCreateGC(impl_->display, impl_->window, 0, nullptr);
    impl_->open = true;
}

XenithWindow::~XenithWindow() {
    if (!impl_) return;
    if (impl_->image) {
        impl_->image->data = nullptr;
        XDestroyImage(impl_->image);
    }
    if (impl_->gc) XFreeGC(impl_->display, impl_->gc);
    if (impl_->window) XDestroyWindow(impl_->display, impl_->window);
    if (impl_->display) XCloseDisplay(impl_->display);
    delete impl_;
}

bool XenithWindow::isOpen() const {
    return impl_ && impl_->open;
}

void XenithWindow::pollEvents() {
    if (!impl_) return;
    while (XPending(impl_->display)) {
        XEvent event{};
        XNextEvent(impl_->display, &event);
        if (event.type == DestroyNotify || event.type == KeyPress) {
            impl_->open = false;
        }
    }
}

void XenithWindow::present(const std::uint32_t* pixels, int width, int height) {
    if (!impl_ || !impl_->open || !pixels) return;
    if (width != impl_->width || height != impl_->height) return;

    if (!impl_->image) {
        const int depth = DefaultDepth(impl_->display, DefaultScreen(impl_->display));
        auto* data = new char[static_cast<std::size_t>(width) * height * 4];
        impl_->image = XCreateImage(
            impl_->display,
            DefaultVisual(impl_->display, DefaultScreen(impl_->display)),
            depth, ZPixmap, 0, data, width, height, 32, 0);
        if (!impl_->image) {
            delete[] data;
            throw std::runtime_error("Could not create XImage");
        }
    }

    std::memcpy(impl_->image->data, pixels,
                static_cast<std::size_t>(width) * height * 4);
    XPutImage(impl_->display, impl_->window, impl_->gc, impl_->image,
              0, 0, 0, 0, static_cast<unsigned>(width), static_cast<unsigned>(height));
    XFlush(impl_->display);
}
