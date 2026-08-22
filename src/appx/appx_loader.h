#pragma once
#include "appx_package.h"
#include <cstdint>
#include <string>
#include <vector>

struct AppxLoadResult {
    std::string executableName;
    std::vector<std::uint8_t> executable;
    AppxManifestInfo manifest;
};

class AppxLoader {
public:
    bool loadFile(const std::string& path, AppxLoadResult& result, std::string& error) const;
};
