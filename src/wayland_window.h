#pragma once
#include <cstdint>
#include <string>
#include "input/input_state.h"
class WaylandWindow {
public:
    WaylandWindow(int width,int height,const char* title,InputState& input);
    ~WaylandWindow();
    WaylandWindow(const WaylandWindow&)=delete;
    WaylandWindow& operator=(const WaylandWindow&)=delete;
    bool isOpen() const; void pollEvents();
    void present(const std::uint32_t* pixels,int width,int height);
    bool menuImportRequested(); bool menuBarHit(int x,int y) const; bool fileMenuOpen() const;
    void setTitle(const std::string& title);
private:
    struct Impl; Impl* impl_; InputState& input_;
};
