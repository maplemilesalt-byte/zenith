#include "renderer.h"

#include <algorithm>
#include <cmath>

Renderer::Renderer(int width, int height)
    : width_(width), height_(height), pixels_(static_cast<std::size_t>(width) * height, 0) {}

void Renderer::clear(std::uint32_t color) {
    std::fill(pixels_.begin(), pixels_.end(), color);
}

void Renderer::putPixel(int x, int y, std::uint32_t color) {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) return;
    pixels_[static_cast<std::size_t>(y) * width_ + x] = color;
}

void Renderer::drawLine(int x0, int y0, int x1, int y1, std::uint32_t color) {
    const int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    const int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;
    while (true) {
        putPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        const int e2 = 2 * error;
        if (e2 >= dy) { error += dy; x0 += sx; }
        if (e2 <= dx) { error += dx; y0 += sy; }
    }
}

void Renderer::drawCube(float angle) {
    constexpr float scale = 220.0f;
    constexpr float cameraDistance = 4.0f;
    const float c = std::cos(angle), s = std::sin(angle);
    const Vertex vertices[] = {
        {-1,-1,-1}, {1,-1,-1}, {1,1,-1}, {-1,1,-1},
        {-1,-1,1}, {1,-1,1}, {1,1,1}, {-1,1,1}
    };
    int projected[8][2]{};
    for (int i = 0; i < 8; ++i) {
        const float rx = vertices[i].x * c - vertices[i].z * s;
        const float rz = vertices[i].x * s + vertices[i].z * c;
        const float perspective = scale / (rz + cameraDistance);
        projected[i][0] = static_cast<int>(rx * perspective + width_ * 0.5f);
        projected[i][1] = static_cast<int>(vertices[i].y * perspective + height_ * 0.5f);
    }
    constexpr int edges[][2] = {
        {0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}
    };
    for (const auto& edge : edges) {
        drawLine(projected[edge[0]][0], projected[edge[0]][1],
                 projected[edge[1]][0], projected[edge[1]][1], 0x00FFFFFF);
    }
}

const std::uint32_t* Renderer::pixels() const { return pixels_.data(); }
int Renderer::width() const { return width_; }
int Renderer::height() const { return height_; }
