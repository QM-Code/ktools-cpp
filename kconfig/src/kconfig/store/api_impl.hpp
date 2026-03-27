#pragma once

#include <filesystem>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <kconfig/json.hpp>
#include <kconfig/store.hpp>
#include <kconfig/store/fs.hpp>
#include <kconfig/store/user.hpp>

namespace kconfig::store::api {

bool Has(std::string_view name);
bool AddMutable(std::string_view name, const kconfig::json::Value& json);
bool AddReadOnly(std::string_view name, const kconfig::json::Value& json);

bool Merge(std::string_view targetName,
           std::initializer_list<std::string_view> sourceNames,
           const kconfig::store::MergeOptions& options = {});

bool Merge(std::string_view targetName,
           const std::vector<std::string>& sourceNames,
           const kconfig::store::MergeOptions& options = {});

bool Unregister(std::string_view name);
std::optional<kconfig::json::Value> Get(std::string_view name, std::string_view path);
bool Set(std::string_view name, std::string_view path, kconfig::json::Value value);
bool Erase(std::string_view name, std::string_view path);
bool SetMutable(std::string_view name);
bool SetReadOnly(std::string_view name);
bool IsMutable(std::string_view name);

} // namespace kconfig::store::api

namespace kconfig::store::fs::api {

bool LoadMutable(std::string_view name,
                 const std::filesystem::path& filename,
                 std::string* error = nullptr);

bool LoadReadOnly(std::string_view name,
                  const std::filesystem::path& filename,
                  std::string* error = nullptr);

bool HasBackingFile(std::string_view name);
std::optional<std::filesystem::path> BackingFilePath(std::string_view name);

bool AttachBackingFile(std::string_view name,
                       const std::filesystem::path& fullFilesystemPath,
                       std::string* error = nullptr);

bool DetachBackingFile(std::string_view name);

bool CreateBackingFile(std::string_view name,
                       const std::filesystem::path& fullFilesystemPath,
                       std::string* error = nullptr);

bool DeleteBackingFile(std::string_view name,
                       std::string* error = nullptr);

bool WriteBackingFile(std::string_view name, std::string* error = nullptr);
bool ReloadBackingFile(std::string_view name, std::string* error = nullptr);

bool HasAssetRoot(std::string_view name);
std::optional<std::filesystem::path> AssetRootPath(std::string_view name);
bool SetAssetRoot(std::string_view name, const std::filesystem::path& fullFilesystemPath);
bool EnsureAssetRootExists(std::string_view name, std::string* error = nullptr);

bool SetSaveIntervalSeconds(std::string_view name, std::optional<double> seconds);
std::optional<double> SaveIntervalSeconds(std::string_view name);
bool FlushPendingWrites(std::string* error = nullptr);

} // namespace kconfig::store::fs::api

namespace kconfig::store::user::api {

bool SetDirname(std::string_view dirname);
bool OverrideConfigFilePath(const std::filesystem::path& fullFilesystemPath);
void ResetConfigLocationOverrides();
std::filesystem::path ConfigFilePath();
bool HasConfigFile();
bool InitializeConfigFile(const kconfig::json::Value& json, std::string* error = nullptr);
bool LoadConfigFile(std::string_view name,
                    const kconfig::store::user::LoadOptions& options = {},
                    std::string* error = nullptr);

} // namespace kconfig::store::user::api
