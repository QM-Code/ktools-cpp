#include "../store.hpp"

#include "kconfig/trace.hpp"
#include "../io.hpp"
#include "api_impl.hpp"
#include "internal.hpp"

#include <chrono>
#include <cctype>
#include <filesystem>
#include <initializer_list>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

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

uint64_t nextLayerRevisionLocked(const std::string& name) {
    if (const auto existing = g_state.namedConfigs.find(name); existing != g_state.namedConfigs.end()) {
        return existing->second.revision + 1;
    }
    return 1;
}

std::string DescribeJsonType(const kconfig::json::Value& value) {
    if (value.is_object()) {
        return "object";
    }
    if (value.is_array()) {
        return "array";
    }
    if (value.is_string()) {
        return "string";
    }
    if (value.is_boolean()) {
        return "boolean";
    }
    if (value.is_number_integer()) {
        return "integer";
    }
    if (value.is_number_float()) {
        return "float";
    }
    if (value.is_null()) {
        return "null";
    }
    return "unknown";
}

std::string JoinSourceNames(std::initializer_list<std::string_view> names) {
    std::ostringstream stream;
    bool first = true;
    for (const auto name : names) {
        if (!first) {
            stream << ", ";
        }
        first = false;
        stream << name;
    }
    return stream.str();
}

std::string JoinSourceNames(const std::vector<std::string>& names) {
    std::ostringstream stream;
    bool first = true;
    for (const auto& name : names) {
        if (!first) {
            stream << ", ";
        }
        first = false;
        stream << name;
    }
    return stream.str();
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

bool addWithMutability(std::string_view name,
                       const kconfig::json::Value& json,
                       bool isMutable) {
    KTRACE("store.requests",
           "Add namespace='{}' mutable={} json_type='{}'",
           std::string(name),
           isMutable,
           DescribeJsonType(json));
    if (!IsValidNamespaceName(name) || !json.is_object()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_state.mutex);
    const std::string key(name);
    std::filesystem::path baseDir{};
    if (const auto existing = g_state.namedConfigs.find(key); existing != g_state.namedConfigs.end()) {
        baseDir = existing->second.baseDir;
    }
    if (const auto assetRoot = g_state.namedAssetRoots.find(key); assetRoot != g_state.namedAssetRoots.end()) {
        baseDir = assetRoot->second;
    } else if (const auto backing = g_state.namedBackingFiles.find(key); backing != g_state.namedBackingFiles.end()) {
        baseDir = backing->second.parent_path();
    }
    const uint64_t layerRevision = nextLayerRevisionLocked(key);

    g_state.namedConfigs[key] = kconfig::store::ConfigLayer{
        json,
        baseDir,
        key,
        isMutable,
        layerRevision
    };
    if (isMutable) {
        auto& saveState = g_state.namedSaveStates[key];
        saveState.pendingSave = false;
    } else {
        g_state.namedSaveStates.erase(key);
    }
    g_state.revision++;
    return true;
}

bool updateMutability(std::string_view name, bool isMutable) {
    KTRACE("store.requests",
           "SetMutability namespace='{}' mutable={}",
           std::string(name),
           isMutable);
    if (!IsValidNamespaceName(name)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_state.mutex);
    const std::string key(name);
    auto configIt = g_state.namedConfigs.find(key);
    if (configIt == g_state.namedConfigs.end()) {
        return false;
    }
    if (configIt->second.isMutable == isMutable) {
        return true;
    }

    configIt->second.isMutable = isMutable;
    configIt->second.revision++;
    if (isMutable) {
        auto& saveState = g_state.namedSaveStates[key];
        saveState.lastSaveTime = std::chrono::steady_clock::now();
        saveState.lastSavedRevision = configIt->second.revision;
        saveState.pendingSave = false;
    } else {
        g_state.namedSaveStates.erase(key);
    }
    g_state.revision++;
    return true;
}

bool storeLoadedJson(std::string_view name,
                     const std::filesystem::path& path,
                     kconfig::json::Value json,
                     bool isMutable) {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    const std::string key(name);
    const auto explicitRootIt = g_state.namedAssetRoots.find(key);
    const std::filesystem::path baseDir =
        (explicitRootIt != g_state.namedAssetRoots.end()) ? explicitRootIt->second : path.parent_path();
    const uint64_t layerRevision = nextLayerRevisionLocked(key);

    g_state.namedConfigs[key] = kconfig::store::ConfigLayer{
        std::move(json),
        baseDir,
        key,
        isMutable,
        layerRevision
    };
    g_state.namedBackingFiles[key] = path;
    if (isMutable) {
        auto& saveState = g_state.namedSaveStates[key];
        saveState.lastSaveTime = std::chrono::steady_clock::now();
        saveState.lastSavedRevision = layerRevision;
        saveState.pendingSave = false;
    } else {
        g_state.namedSaveStates.erase(key);
    }
    g_state.revision++;
    return true;
}

template <typename Range>
bool mergeRange(std::string_view targetName,
                const Range& sourceNames,
                const kconfig::store::MergeOptions& options,
                const std::string& joinedSources) {
    KTRACE("store",
           "Merge requested: [{}] -> '{}' (enable store.requests for details)",
           joinedSources,
           std::string(targetName));
    KTRACE("store.requests",
           "Merge target='{}' sources=[{}] mutable={}",
           std::string(targetName),
           joinedSources,
           options.mode == kconfig::store::Mode::Mutable);
    if (!IsValidNamespaceName(targetName)) {
        return false;
    }

    kconfig::json::Value merged = kconfig::json::Object();
    {
        std::lock_guard<std::mutex> lock(g_state.mutex);
        bool hadSource = false;
        for (const auto& sourceName : sourceNames) {
            hadSource = true;
            KTRACE("store.requests",
                   "Merge target='{}' source='{}' -> lookup",
                   std::string(targetName),
                   std::string(sourceName));
            const auto it = g_state.namedConfigs.find(std::string(sourceName));
            if (it == g_state.namedConfigs.end() || !it->second.json.is_object()) {
                return false;
            }
            kconfig::io::MergeJsonObjects(merged, it->second.json);
        }
        if (!hadSource) {
            return false;
        }

        const std::string key(targetName);
        std::filesystem::path baseDir{};
        if (const auto existing = g_state.namedConfigs.find(key); existing != g_state.namedConfigs.end()) {
            baseDir = existing->second.baseDir;
        }
        if (const auto explicitRootIt = g_state.namedAssetRoots.find(key); explicitRootIt != g_state.namedAssetRoots.end()) {
            baseDir = explicitRootIt->second;
        } else if (const auto backingIt = g_state.namedBackingFiles.find(key); backingIt != g_state.namedBackingFiles.end()) {
            baseDir = backingIt->second.parent_path();
        }
        const uint64_t layerRevision = nextLayerRevisionLocked(key);

        const bool isMutable = (options.mode == kconfig::store::Mode::Mutable);
        g_state.namedConfigs[key] = kconfig::store::ConfigLayer{
            std::move(merged),
            baseDir,
            key,
            isMutable,
            layerRevision
        };
        if (isMutable) {
            auto& saveState = g_state.namedSaveStates[key];
            saveState.pendingSave = (g_state.namedBackingFiles.find(key) != g_state.namedBackingFiles.end());
        } else {
            g_state.namedSaveStates.erase(key);
        }
        g_state.revision++;
    }
    return true;
}

} // namespace

namespace kconfig::store::api {

bool Has(std::string_view name) {
    KTRACE("store.requests", "Has namespace='{}'", std::string(name));
    if (!IsValidNamespaceName(name)) {
        return false;
    }
    std::lock_guard<std::mutex> lock(g_state.mutex);
    return g_state.namedConfigs.find(std::string(name)) != g_state.namedConfigs.end();
}

bool AddMutable(std::string_view name, const kconfig::json::Value& json) {
    return addWithMutability(name, json, true);
}

bool AddReadOnly(std::string_view name, const kconfig::json::Value& json) {
    return addWithMutability(name, json, false);
}

bool Merge(std::string_view targetName,
           std::initializer_list<std::string_view> sourceNames,
           const kconfig::store::MergeOptions& options) {
    return mergeRange(targetName, sourceNames, options, JoinSourceNames(sourceNames));
}

bool Merge(std::string_view targetName,
           const std::vector<std::string>& sourceNames,
           const kconfig::store::MergeOptions& options) {
    return mergeRange(targetName, sourceNames, options, JoinSourceNames(sourceNames));
}

bool Unregister(std::string_view name) {
    KTRACE("store.requests",
           "Unregister namespace='{}'",
           std::string(name));
    if (!IsValidNamespaceName(name)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_state.mutex);
    const std::string key(name);
    const bool removedConfig = (g_state.namedConfigs.erase(key) > 0);
    const bool removedBacking = (g_state.namedBackingFiles.erase(key) > 0);
    const bool removedAssetRoot = (g_state.namedAssetRoots.erase(key) > 0);
    const bool removedSaveState = (g_state.namedSaveStates.erase(key) > 0);
    if (!removedConfig && !removedBacking && !removedAssetRoot && !removedSaveState) {
        return false;
    }

    g_state.revision++;
    return true;
}

bool SetMutable(std::string_view name) {
    return updateMutability(name, true);
}

bool SetReadOnly(std::string_view name) {
    return updateMutability(name, false);
}

bool IsMutable(std::string_view name) {
    KTRACE("store.requests", "IsMutable namespace='{}'", std::string(name));
    if (!IsValidNamespaceName(name)) {
        return false;
    }
    std::lock_guard<std::mutex> lock(g_state.mutex);
    const auto it = g_state.namedConfigs.find(std::string(name));
    return it != g_state.namedConfigs.end() && it->second.isMutable;
}

} // namespace kconfig::store::api

namespace kconfig::store::fs::api {

bool LoadMutable(std::string_view name,
                 const std::filesystem::path& filename,
                 std::string* error) {
    KTRACE("store",
           "LoadMutable requested: namespace='{}' path='{}' (enable store.requests for details)",
           std::string(name),
           filename.string());
    KTRACE("store.requests",
           "LoadMutable namespace='{}' filename='{}'",
           std::string(name),
           filename.string());
    if (!IsValidNamespaceName(name)) {
        if (error) {
            *error = "namespace is invalid";
        }
        return false;
    }

    std::filesystem::path path = canonicalizeFullPath(filename);
    const auto readResult = kconfig::io::ReadJsonFile(path);
    if (!readResult.json.has_value()) {
        if (error) {
            switch (readResult.error) {
            case kconfig::io::JsonReadError::NotFound:
                *error = "file not found: " + path.string();
                break;
            case kconfig::io::JsonReadError::OpenFailed:
                *error = "failed to open file: " + path.string();
                break;
            case kconfig::io::JsonReadError::ParseFailed:
                *error = "failed to parse file: " + readResult.message;
                break;
            case kconfig::io::JsonReadError::None:
                *error = "failed to read file";
                break;
            }
        }
        return false;
    }
    if (!readResult.json->is_object()) {
        if (error) {
            *error = "loaded JSON root must be an object";
        }
        return false;
    }

    return storeLoadedJson(name, path, std::move(*readResult.json), true);
}

bool LoadReadOnly(std::string_view name,
                  const std::filesystem::path& filename,
                  std::string* error) {
    KTRACE("store",
           "LoadReadOnly requested: namespace='{}' path='{}' (enable store.requests for details)",
           std::string(name),
           filename.string());
    KTRACE("store.requests",
           "LoadReadOnly namespace='{}' filename='{}'",
           std::string(name),
           filename.string());
    if (!IsValidNamespaceName(name)) {
        if (error) {
            *error = "namespace is invalid";
        }
        return false;
    }

    std::filesystem::path path = canonicalizeFullPath(filename);
    const auto readResult = kconfig::io::ReadJsonFile(path);
    if (!readResult.json.has_value()) {
        if (error) {
            switch (readResult.error) {
            case kconfig::io::JsonReadError::NotFound:
                *error = "file not found: " + path.string();
                break;
            case kconfig::io::JsonReadError::OpenFailed:
                *error = "failed to open file: " + path.string();
                break;
            case kconfig::io::JsonReadError::ParseFailed:
                *error = "failed to parse file: " + readResult.message;
                break;
            case kconfig::io::JsonReadError::None:
                *error = "failed to read file";
                break;
            }
        }
        return false;
    }
    if (!readResult.json->is_object()) {
        if (error) {
            *error = "loaded JSON root must be an object";
        }
        return false;
    }

    return storeLoadedJson(name, path, std::move(*readResult.json), false);
}

} // namespace kconfig::store::fs::api
