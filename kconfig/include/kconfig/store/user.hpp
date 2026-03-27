#pragma once

#include <filesystem>
#include <string>
#include <string_view>

#include <kconfig/json.hpp>

namespace kconfig::store::user {

enum class LoadMode {
    ReadOnly,
    Mutable
};

struct LoadOptions {
    LoadMode mode = LoadMode::Mutable;
};

bool SetDirname(std::string_view dirname);
bool OverrideConfigFilePath(const std::filesystem::path& path);
void ResetConfigLocationOverrides();
std::filesystem::path ConfigFilePath();
bool HasConfigFile();

bool InitializeConfigFile(const kconfig::json::Value& json,
                          std::string* error = nullptr);

bool LoadConfigFile(std::string_view name,
                    const LoadOptions& options = {},
                    std::string* error = nullptr);

} // namespace kconfig::store::user
