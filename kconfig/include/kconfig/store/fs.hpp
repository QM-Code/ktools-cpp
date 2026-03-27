#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace kconfig::store::fs {

bool LoadMutable(std::string_view name,
                 const std::filesystem::path& path,
                 std::string* error = nullptr);

bool LoadReadOnly(std::string_view name,
                  const std::filesystem::path& path,
                  std::string* error = nullptr);

bool HasBackingFile(std::string_view name);
std::optional<std::filesystem::path> BackingFilePath(std::string_view name);

bool AttachBackingFile(std::string_view name,
                       const std::filesystem::path& path,
                       std::string* error = nullptr);

bool DetachBackingFile(std::string_view name);

bool CreateBackingFile(std::string_view name,
                       const std::filesystem::path& path,
                       std::string* error = nullptr);

bool DeleteBackingFile(std::string_view name,
                       std::string* error = nullptr);

bool WriteBackingFile(std::string_view name,
                      std::string* error = nullptr);

bool ReloadBackingFile(std::string_view name,
                       std::string* error = nullptr);

bool HasAssetRoot(std::string_view name);
std::optional<std::filesystem::path> AssetRootPath(std::string_view name);

bool SetAssetRoot(std::string_view name,
                  const std::filesystem::path& path);

bool EnsureAssetRootExists(std::string_view name,
                           std::string* error = nullptr);

bool SetSaveIntervalSeconds(std::string_view name,
                            std::optional<double> seconds);

std::optional<double> SaveIntervalSeconds(std::string_view name);

bool FlushPendingWrites(std::string* error = nullptr);

} // namespace kconfig::store::fs
