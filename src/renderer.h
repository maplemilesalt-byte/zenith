#pragma once
#include <cstdint>
#include <vector>
class GuestMemory;
class Renderer {
public:
    Renderer(int width,int height);
    void clear(std::uint32_t color);
    void drawCube(float angle);
    bool drawGuestFramebuffer(const GuestMemory& memory,std::uint64_t address,int guestWidth,int guestHeight);
    const std::uint32_t* pixels() const;
    int width() const; int height() const;
private:
    struct Vertex{float x,y,z;};
    void putPixel(int x,int y,std::uint32_t color);
    void drawLine(int x0,int y0,int x1,int y1,std::uint32_t color);
    int width_,height_; std::vector<std::uint32_t> pixels_;
};
