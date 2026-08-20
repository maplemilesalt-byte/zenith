#pragma once

#include <cstdint>

class XenithWindow {
public:
    XenithWindow(int width, int height, const char* title);
    ~XenithWindow();

    XenithWindow(const XenithWindow&) = delete;
    XenithWindow& operator=(const XenithWindow&) = delete;

    bool isOpen() const;
    void pollEvents();
    void present(const std::uint32_t* pixels, int width, int height);

private:
    struct Impl;
    Impl* impl_;
};
