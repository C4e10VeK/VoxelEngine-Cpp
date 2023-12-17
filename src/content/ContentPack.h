#ifndef CONTENT_CONTENT_PACK_H_
#define CONTENT_CONTENT_PACK_H_

#include <string>
#include <vector>
#include <filesystem>

struct ContentPack {
    std::string id = "none";
    std::string title = "untitled";
    std::string version = "0.0";
    std::filesystem::path folder;

    std::filesystem::path getContentFile() const;

    static const std::string PACKAGE_FILENAME;
    static const std::string CONTENT_FILENAME;
    static bool is_pack(std::filesystem::path folder);
    static ContentPack read(std::filesystem::path folder);
    static void scan(std::filesystem::path folder, 
                     std::vector<ContentPack>& packs);
};

#endif // CONTENT_CONTENT_PACK_H_
