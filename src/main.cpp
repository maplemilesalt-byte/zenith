#include "renderer.h"
#include "window.h"

#include <chrono>
#include <iostream>
#include <thread>

int main() {
    constexpr int width = 1280;
    constexpr int height = 720;

    std::cout << "Xenith 0.1.0\n";
    std::cout << "Initializing renderer...\n";

    XenithWindow window(width, height, "Xenith - Xbox Series X|S Emulator");
    Renderer renderer(width, height);

    float angle = 0.0f;

    while (window.isOpen()) {
        window.pollEvents();
        renderer.clear(0x00101018);
        renderer.drawCube(angle);
        window.present(renderer.pixels(), renderer.width(), renderer.height());
        angle += 0.01f;
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    std::cout << "Xenith shutting down.\n";
    return 0;
}
