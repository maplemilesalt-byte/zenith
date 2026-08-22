# Zenith Xbox UWP host

First-stage Xbox/UWP integration for Zenith.

This directory contains the native C++/WinRT host skeleton. The host is intentionally small: it initializes the Zenith core and leaves framebuffer/input integration for later stages.

## Intended build environment

- Visual Studio with the Universal Windows Platform development workload
- Windows SDK suitable for Xbox UWP development
- C++/WinRT

The existing Linux desktop build remains unchanged; this directory is a separate Xbox platform target.
