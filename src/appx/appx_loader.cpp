#include "appx_loader.h"

namespace {
bool hasElfMagic(const std::vector<std::uint8_t>& data) {
    return data.size() >= 4 && data[0] == 0x7f && data[1] == 'E' && data[2] == 'L' && data[3] == 'F';
}
}

bool AppxLoader::loadFile(const std::string& path, AppxLoadResult& result, std::string& error) const {
    result = {};
    AppxPackage package;
    if (!package.open(path, error)) return false;
    result.manifest = package.manifest();
    const std::string executable = result.manifest.executable;
    if (executable.empty()) { error = "APPX manifest does not specify an executable"; return false; }
    if (!package.hasFile(executable)) { error = "APPX executable not found: " + executable; return false; }
    if (!package.readFile(executable, result.executable, error)) return false;
    if (!hasElfMagic(result.executable)) { error = "APPX executable is not an ELF binary"; return false; }
    result.executableName = executable;
    return true;
}
