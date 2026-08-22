#pragma once

#include <cstdint>
#include <string>

class XenithWindow {
public:
    XenithWindow(int width, int height, const char* title);
    ~XenithWindow();

    XenithWindow(const XenithWindow&) = delete;
    XenithWindow& operator=(const XenithWindow&) = delete;

    bool isOpen() const;
    void pollEvents();
    void present(const std::uint32_t* pixels, int width, int height);
    bool menuImportRequested();
    bool menuBarHit(int x, int y) const;
    bool fileMenuOpen() const;
    void setTitle(const std::string& title);

private:
    struct Impl;
    Impl* impl_;
};
