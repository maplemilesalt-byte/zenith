#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct AppxFileEntry {
    std::string name;
    std::uint32_t compressedSize = 0;
    std::uint32_t uncompressedSize = 0;
    std::uint32_t localHeaderOffset = 0;
    std::uint16_t method = 0;
};

struct AppxManifestInfo {
    std::string identityName;
    std::string publisher;
    std::string version;
    std::string displayName;
    std::string description;
    std::string executable;
};

class AppxPackage {
public:
    bool open(const std::string& path, std::string& error);
    void close();
    bool isOpen() const;
    const std::string& path() const { return path_; }
    const std::vector<AppxFileEntry>& files() const { return files_; }
    const AppxManifestInfo& manifest() const { return manifest_; }
    bool hasFile(const std::string& name) const;
    bool readFile(const std::string& name, std::vector<std::uint8_t>& data, std::string& error) const;
    bool readManifest(std::string& xml, std::string& error) const;

private:
    bool parseCentralDirectory(std::string& error);
    bool parseManifest(const std::string& xml, std::string& error);
    std::string path_;
    std::vector<std::uint8_t> archive_;
    std::vector<AppxFileEntry> files_;
    AppxManifestInfo manifest_;
};
