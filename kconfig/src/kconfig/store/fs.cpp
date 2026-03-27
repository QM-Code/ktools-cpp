#include <kconfig/store/fs.hpp>

#include "api_impl.hpp"

namespace kconfig::store::fs {

bool LoadMutable(std::string_view name,
                 const std::filesystem::path& path,
                 std::string* error) {
    return api::LoadMutable(name, path, error);
}

bool LoadReadOnly(std::string_view name,
                  const std::filesystem::path& path,
                  std::string* error) {
    return api::LoadReadOnly(name, path, error);
}

bool HasBackingFile(std::string_view name) {
    return api::HasBackingFile(name);
}

std::optional<std::filesystem::path> BackingFilePath(std::string_view name) {
    return api::BackingFilePath(name);
}

bool AttachBackingFile(std::string_view name,
                       const std::filesystem::path& path,
                       std::string* error) {
    return api::AttachBackingFile(name, path, error);
}

bool DetachBackingFile(std::string_view name) {
    return api::DetachBackingFile(name);
}

bool CreateBackingFile(std::string_view name,
                       const std::filesystem::path& path,
                       std::string* error) {
    return api::CreateBackingFile(name, path, error);
}

bool DeleteBackingFile(std::string_view name,
                       std::string* error) {
    return api::DeleteBackingFile(name, error);
}

bool WriteBackingFile(std::string_view name,
                      std::string* error) {
    return api::WriteBackingFile(name, error);
}

bool ReloadBackingFile(std::string_view name,
                       std::string* error) {
    return api::ReloadBackingFile(name, error);
}

bool HasAssetRoot(std::string_view name) {
    return api::HasAssetRoot(name);
}

std::optional<std::filesystem::path> AssetRootPath(std::string_view name) {
    return api::AssetRootPath(name);
}

bool SetAssetRoot(std::string_view name,
                  const std::filesystem::path& path) {
    return api::SetAssetRoot(name, path);
}

bool EnsureAssetRootExists(std::string_view name,
                           std::string* error) {
    return api::EnsureAssetRootExists(name, error);
}

bool SetSaveIntervalSeconds(std::string_view name,
                            std::optional<double> seconds) {
    return api::SetSaveIntervalSeconds(name, seconds);
}

std::optional<double> SaveIntervalSeconds(std::string_view name) {
    return api::SaveIntervalSeconds(name);
}

bool FlushPendingWrites(std::string* error) {
    return api::FlushPendingWrites(error);
}

} // namespace kconfig::store::fs
