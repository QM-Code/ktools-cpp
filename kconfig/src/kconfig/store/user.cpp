#include <kconfig/store/user.hpp>

#include "api_impl.hpp"

namespace kconfig::store::user {

bool SetDirname(std::string_view dirname) {
    return api::SetDirname(dirname);
}

bool OverrideConfigFilePath(const std::filesystem::path& path) {
    return api::OverrideConfigFilePath(path);
}

void ResetConfigLocationOverrides() {
    api::ResetConfigLocationOverrides();
}

std::filesystem::path ConfigFilePath() {
    return api::ConfigFilePath();
}

bool HasConfigFile() {
    return api::HasConfigFile();
}

bool InitializeConfigFile(const kconfig::json::Value& json,
                          std::string* error) {
    return api::InitializeConfigFile(json, error);
}

bool LoadConfigFile(std::string_view name,
                    const LoadOptions& options,
                    std::string* error) {
    return api::LoadConfigFile(name, options, error);
}

} // namespace kconfig::store::user
