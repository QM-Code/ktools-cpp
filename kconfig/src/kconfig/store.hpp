#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <kconfig/json.hpp>

namespace kconfig::store {

struct ConfigLayer {
    kconfig::json::Value json;
    std::filesystem::path baseDir;
    std::string label;
    bool isMutable = true;
    uint64_t revision = 0;
};

// Private helpers for store implementation files.
bool isRootPath(std::string_view path);
const kconfig::json::Value* resolvePath(const kconfig::json::Value& root, std::string_view path);
const kconfig::json::Value* findNamedValueLocked(std::string_view name);
kconfig::json::Value* findMutableNamedValueLocked(std::string_view name);
bool writeJsonFileUnlocked(const std::filesystem::path& path,
                           const kconfig::json::Value& json,
                           std::string* error);
bool setValueAtPath(kconfig::json::Value& root, std::string_view path, kconfig::json::Value value);
bool eraseValueAtPath(kconfig::json::Value& root, std::string_view path);

} // namespace kconfig::store
