#include <kconfig.hpp>

#include "kconfig/trace.hpp"
#include "../store.hpp"

#include "../io.hpp"
#include "api_impl.hpp"
#include "internal.hpp"

#include <chrono>
#include <cctype>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

namespace {

using kconfig::store::internal::g_state;

bool IsValidNamespaceName(std::string_view name) {
    if (name.empty()) {
        return false;
    }
    for (const char ch : name) {
        if (std::isspace(static_cast<unsigned char>(ch)) != 0 || ch == '.') {
            return false;
        }
    }
    return true;
}

std::filesystem::path canonicalizeFullPath(std::filesystem::path path) {
    if (path.empty()) {
        return {};
    }
    if (path.is_relative()) {
        path = std::filesystem::absolute(path);
    }
    return kconfig::io::Canonicalize(path);
}

} // namespace

namespace kconfig::store::fs::api {

bool HasBackingFile(std::string_view name) {
    KTRACE("store.requests", "HasBackingFile namespace='{}'", std::string(name));
    if (!IsValidNamespaceName(name)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_state.mutex);
    return g_state.namedBackingFiles.find(std::string(name)) != g_state.namedBackingFiles.end();
}

std::optional<std::filesystem::path> BackingFilePath(std::string_view name) {
    KTRACE("store.requests", "BackingFilePath namespace='{}'", std::string(name));
    if (!IsValidNamespaceName(name)) {
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(g_state.mutex);
    const auto it = g_state.namedBackingFiles.find(std::string(name));
    if (it == g_state.namedBackingFiles.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool AttachBackingFile(std::string_view name,
                       const std::filesystem::path& fullFilesystemPath,
                       std::string* error) {
    KTRACE("store",
           "AttachBackingFile requested: namespace='{}' path='{}' (enable store.requests for details)",
           std::string(name),
           fullFilesystemPath.string());
    KTRACE("store.requests",
           "AttachBackingFile namespace='{}' path='{}'",
           std::string(name),
           fullFilesystemPath.string());
    if (!IsValidNamespaceName(name)) {
        if (error) {
            *error = "namespace is invalid";
        }
        return false;
    }

    const std::filesystem::path canonical = canonicalizeFullPath(fullFilesystemPath);
    std::error_code ec;
    if (canonical.empty() || !std::filesystem::exists(canonical, ec) || ec || !std::filesystem::is_regular_file(canonical, ec) || ec) {
        if (error) {
            *error = "backing file must exist: " + canonical.string();
        }
        return false;
    }

    std::lock_guard<std::mutex> lock(g_state.mutex);
    const std::string key(name);
    const auto it = g_state.namedConfigs.find(key);
    if (it == g_state.namedConfigs.end()) {
        if (error) {
            *error = "named config not found";
        }
        return false;
    }

    g_state.namedBackingFiles[key] = canonical;
    if (g_state.namedAssetRoots.find(key) == g_state.namedAssetRoots.end()) {
        g_state.namedConfigs[key].baseDir = canonical.parent_path();
    }
    if (it->second.isMutable) {
        auto& saveState = g_state.namedSaveStates[key];
        saveState.pendingSave = true;
    } else {
        g_state.namedSaveStates.erase(key);
    }
    g_state.revision++;
    return true;
}

bool DetachBackingFile(std::string_view name) {
    KTRACE("store.requests",
           "DetachBackingFile namespace='{}'",
           std::string(name));
    if (!IsValidNamespaceName(name)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_state.mutex);
    const std::string key(name);
    if (g_state.namedBackingFiles.erase(key) == 0) {
        return false;
    }

    if (g_state.namedAssetRoots.find(key) == g_state.namedAssetRoots.end()) {
        if (auto configIt = g_state.namedConfigs.find(key); configIt != g_state.namedConfigs.end()) {
            configIt->second.baseDir = std::filesystem::path{};
        }
    }
    if (auto saveIt = g_state.namedSaveStates.find(key); saveIt != g_state.namedSaveStates.end()) {
        saveIt->second.pendingSave = false;
    }

    g_state.revision++;
    return true;
}

bool CreateBackingFile(std::string_view name,
                       const std::filesystem::path& fullFilesystemPath,
                       std::string* error) {
    KTRACE("store",
           "CreateBackingFile requested: namespace='{}' path='{}' (enable store.requests for details)",
           std::string(name),
           fullFilesystemPath.string());
    KTRACE("store.requests",
           "CreateBackingFile namespace='{}' path='{}'",
           std::string(name),
           fullFilesystemPath.string());
    if (!IsValidNamespaceName(name)) {
        if (error) {
            *error = "namespace is invalid";
        }
        return false;
    }

    const std::filesystem::path canonical = canonicalizeFullPath(fullFilesystemPath);
    if (canonical.empty()) {
        if (error) {
            *error = "backing file path is empty";
        }
        return false;
    }

    {
        std::error_code existsEc;
        if (std::filesystem::exists(canonical, existsEc) && !existsEc) {
            if (error) {
                *error = "backing file already exists: " + canonical.string();
            }
            return false;
        }
    }

    kconfig::json::Value json;
    bool isMutable = false;
    uint64_t revisionSnapshot = 0;
    {
        std::lock_guard<std::mutex> lock(g_state.mutex);
        const auto it = g_state.namedConfigs.find(std::string(name));
        if (it == g_state.namedConfigs.end()) {
            if (error) {
                *error = "named config not found";
            }
            return false;
        }
        json = it->second.json;
        isMutable = it->second.isMutable;
        revisionSnapshot = it->second.revision;
    }

    if (!kconfig::store::writeJsonFileUnlocked(canonical, json, error)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_state.mutex);
    const std::string key(name);
    auto configIt = g_state.namedConfigs.find(key);
    if (configIt == g_state.namedConfigs.end()) {
        return false;
    }

    g_state.namedBackingFiles[key] = canonical;
    if (g_state.namedAssetRoots.find(key) == g_state.namedAssetRoots.end()) {
        configIt->second.baseDir = canonical.parent_path();
    }
    if (isMutable) {
        auto& saveState = g_state.namedSaveStates[key];
        saveState.lastSaveTime = std::chrono::steady_clock::now();
        saveState.lastSavedRevision = revisionSnapshot;
        saveState.pendingSave = false;
    } else {
        g_state.namedSaveStates.erase(key);
    }
    g_state.revision++;
    return true;
}

bool DeleteBackingFile(std::string_view name,
                       std::string* error) {
    KTRACE("store",
           "DeleteBackingFile requested: namespace='{}' (enable store.requests for details)",
           std::string(name));
    KTRACE("store.requests",
           "DeleteBackingFile namespace='{}'",
           std::string(name));
    if (!IsValidNamespaceName(name)) {
        if (error) {
            *error = "namespace is invalid";
        }
        return false;
    }

    std::filesystem::path backingPath;
    {
        std::lock_guard<std::mutex> lock(g_state.mutex);
        const auto it = g_state.namedBackingFiles.find(std::string(name));
        if (it == g_state.namedBackingFiles.end()) {
            if (error) {
                *error = "no backing file configured";
            }
            return false;
        }
        backingPath = it->second;
    }

    std::error_code removeEc;
    const bool removed = std::filesystem::remove(backingPath, removeEc);
    if (removeEc) {
        if (error) {
            *error = "failed to remove backing file '" + backingPath.string() + "': " + removeEc.message();
        }
        return false;
    }
    if (!removed) {
        if (error) {
            *error = "backing file was not removed: " + backingPath.string();
        }
        return false;
    }

    return DetachBackingFile(name);
}

bool HasAssetRoot(std::string_view name) {
    KTRACE("store.requests", "HasAssetRoot namespace='{}'", std::string(name));
    if (!IsValidNamespaceName(name)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_state.mutex);
    return g_state.namedAssetRoots.find(std::string(name)) != g_state.namedAssetRoots.end();
}

std::optional<std::filesystem::path> AssetRootPath(std::string_view name) {
    KTRACE("store.requests", "AssetRootPath namespace='{}'", std::string(name));
    if (!IsValidNamespaceName(name)) {
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(g_state.mutex);
    const auto it = g_state.namedAssetRoots.find(std::string(name));
    if (it == g_state.namedAssetRoots.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool SetAssetRoot(std::string_view name,
                  const std::filesystem::path& fullFilesystemPath) {
    KTRACE("store.requests",
           "SetAssetRoot namespace='{}' path='{}'",
           std::string(name),
           fullFilesystemPath.string());
    if (!IsValidNamespaceName(name)) {
        return false;
    }

    const std::filesystem::path canonical = canonicalizeFullPath(fullFilesystemPath);
    if (canonical.empty()) {
        return false;
    }

    std::error_code ec;
    if (!std::filesystem::exists(canonical, ec) || ec) {
        const auto log = kconfig::GetTraceLogger();
        log.warn("asset root '{}' does not exist yet", canonical.string());
    }

    std::lock_guard<std::mutex> lock(g_state.mutex);
    const auto it = g_state.namedConfigs.find(std::string(name));
    if (it == g_state.namedConfigs.end()) {
        return false;
    }

    const std::string key(name);
    g_state.namedAssetRoots[key] = canonical;
    g_state.namedConfigs[key].baseDir = canonical;
    g_state.revision++;
    return true;
}

bool EnsureAssetRootExists(std::string_view name,
                           std::string* error) {
    KTRACE("store.requests",
           "EnsureAssetRootExists namespace='{}'",
           std::string(name));
    const auto assetRoot = AssetRootPath(name);
    if (!assetRoot.has_value()) {
        if (error) {
            *error = "no asset root configured";
        }
        return false;
    }

    std::error_code ec;
    std::filesystem::create_directories(*assetRoot, ec);
    if (ec) {
        if (error) {
            *error = "failed to create asset root '" + assetRoot->string() + "': " + ec.message();
        }
        return false;
    }
    return true;
}

} // namespace kconfig::store::fs::api
