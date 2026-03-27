#include "internal.hpp"

#include "kconfig/trace.hpp"
#include "../io.hpp"
#include "api_impl.hpp"

#include <kconfig/store/fs.hpp>
#include <kconfig/store/user.hpp>

#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {

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

std::string DescribeSaveInterval(const std::optional<double>& seconds) {
    if (!seconds.has_value()) {
        return "unset";
    }
    std::ostringstream stream;
    stream << *seconds << "s";
    return stream.str();
}

void roundFloatValues(kconfig::json::Value& node) {
    if (node.is_object()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            roundFloatValues(it.value());
        }
        return;
    }

    if (node.is_array()) {
        for (auto& entry : node) {
            roundFloatValues(entry);
        }
        return;
    }

    if (node.is_number_float()) {
        const double value = node.get<double>();
        const double rounded = std::round(value * 100.0) / 100.0;
        node = rounded;
    }
}

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

constexpr std::string_view kUserConfigFilename = "config.json";
constexpr std::string_view kDefaultUserConfigDirname = "app";

const char* UserLoadModeName(kconfig::store::user::LoadMode mode) {
    switch (mode) {
    case kconfig::store::user::LoadMode::ReadOnly:
        return "readonly";
    case kconfig::store::user::LoadMode::Mutable:
        return "mutable";
    }
    return "unknown";
}

std::filesystem::path DetectDefaultUserConfigDirectory() {
    std::filesystem::path base;

#if defined(_WIN32)
    if (const char* appData = std::getenv("APPDATA"); appData && *appData) {
        base = appData;
    } else if (const char* userProfile = std::getenv("USERPROFILE"); userProfile && *userProfile) {
        base = std::filesystem::path(userProfile) / "AppData" / "Roaming";
    }
#elif defined(__APPLE__)
    if (const char* home = std::getenv("HOME"); home && *home) {
        base = std::filesystem::path(home) / "Library" / "Application Support";
    }
#else
    if (const char* xdg = std::getenv("XDG_CONFIG_HOME"); xdg && *xdg) {
        base = xdg;
    } else if (const char* home = std::getenv("HOME"); home && *home) {
        base = std::filesystem::path(home) / ".config";
    }
#endif

    if (base.empty()) {
        throw std::runtime_error("Unable to determine user configuration directory: no home path detected");
    }

    return kconfig::io::Canonicalize(base / std::string(kDefaultUserConfigDirname));
}

std::filesystem::path DefaultUserConfigDirectory() {
    static const std::filesystem::path defaultDir = DetectDefaultUserConfigDirectory();
    return defaultDir;
}

std::filesystem::path DefaultUserConfigFilePath() {
    return DefaultUserConfigDirectory() / std::string(kUserConfigFilename);
}

std::filesystem::path CurrentUserConfigFilePath() {
    std::optional<std::filesystem::path> fileOverride;
    std::optional<std::filesystem::path> dirOverride;
    {
        std::lock_guard<std::mutex> lock(kconfig::store::internal::g_state.mutex);
        fileOverride = kconfig::store::internal::g_state.userConfigFilePathOverride;
        dirOverride = kconfig::store::internal::g_state.userConfigDirOverride;
    }
    if (fileOverride.has_value()) {
        return *fileOverride;
    }
    if (dirOverride.has_value()) {
        return *dirOverride / std::string(kUserConfigFilename);
    }
    return DefaultUserConfigFilePath();
}

} // namespace

namespace kconfig::store {

bool writeJsonFileUnlocked(const std::filesystem::path& path,
                           const kconfig::json::Value& json,
                           std::string* error) {
    KTRACE("store.requests",
           "WriteJsonFile path='{}' json_type='{}'",
           path.string(),
           DescribeJsonType(json));
    if (path.empty()) {
        if (error) {
            *error = "Config path is empty.";
        }
        return false;
    }

    kconfig::json::Value rounded = json;
    roundFloatValues(rounded);
    KTRACE("config", "writing config '{}'", path.string());
    if (!kconfig::io::WriteJsonFile(path, rounded, error)) {
        if (error && error->empty()) {
            *error = "Failed to write config file.";
        }
        return false;
    }
    return true;
}

} // namespace kconfig::store

namespace kconfig::store::fs::api {

bool WriteBackingFile(std::string_view name, std::string* error) {
    KTRACE("store",
           "WriteBackingFile requested: namespace='{}' (enable store.requests for details)",
           std::string(name));
    KTRACE("store.requests",
           "WriteBackingFile namespace='{}'",
           std::string(name));
    if (!IsValidNamespaceName(name)) {
        if (error) {
            *error = "namespace is invalid";
        }
        return false;
    }

    std::string key;
    std::filesystem::path path;
    kconfig::json::Value value;
    uint64_t revisionSnapshot = 0;
    bool isMutable = false;

    {
        std::lock_guard<std::mutex> lock(internal::g_state.mutex);
        key = std::string(name);
        const auto configIt = internal::g_state.namedConfigs.find(key);
        if (configIt == internal::g_state.namedConfigs.end()) {
            if (error) {
                *error = "named config not found";
            }
            return false;
        }

        const auto backing = internal::g_state.namedBackingFiles.find(key);
        if (backing == internal::g_state.namedBackingFiles.end()) {
            if (error) {
                *error = "no backing file configured";
            }
            return false;
        }

        value = configIt->second.json;
        revisionSnapshot = configIt->second.revision;
        isMutable = configIt->second.isMutable;
        path = backing->second;
    }

    if (!writeJsonFileUnlocked(path, value, error)) {
        return false;
    }

    if (!isMutable) {
        return true;
    }

    const auto saveTime = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(internal::g_state.mutex);
    const auto configIt = internal::g_state.namedConfigs.find(key);
    const auto saveIt = internal::g_state.namedSaveStates.find(key);
    const auto backingIt = internal::g_state.namedBackingFiles.find(key);
    if (configIt == internal::g_state.namedConfigs.end()
        || saveIt == internal::g_state.namedSaveStates.end()
        || backingIt == internal::g_state.namedBackingFiles.end()) {
        return true;
    }

    auto& saveState = saveIt->second;
    saveState.lastSaveTime = saveTime;
    saveState.lastSavedRevision = revisionSnapshot;
    saveState.pendingSave = (configIt->second.revision != revisionSnapshot) || (backingIt->second != path);
    return true;
}

bool ReloadBackingFile(std::string_view name, std::string* error) {
    KTRACE("store",
           "ReloadBackingFile requested: namespace='{}' (enable store.requests for details)",
           std::string(name));
    KTRACE("store.requests",
           "ReloadBackingFile namespace='{}'",
           std::string(name));
    if (!IsValidNamespaceName(name)) {
        if (error) {
            *error = "namespace is invalid";
        }
        return false;
    }

    std::filesystem::path path;
    {
        std::lock_guard<std::mutex> lock(internal::g_state.mutex);
        const auto backingIt = internal::g_state.namedBackingFiles.find(std::string(name));
        if (backingIt == internal::g_state.namedBackingFiles.end()) {
            if (error) {
                *error = "no backing file configured";
            }
            return false;
        }
        path = backingIt->second;
    }

    const auto readResult = kconfig::io::ReadJsonFile(path);
    if (!readResult.json.has_value()) {
        if (error) {
            switch (readResult.error) {
            case kconfig::io::JsonReadError::NotFound:
                *error = "backing file not found: " + path.string();
                break;
            case kconfig::io::JsonReadError::OpenFailed:
                *error = "failed to open backing file: " + path.string();
                break;
            case kconfig::io::JsonReadError::ParseFailed:
                *error = "failed to parse backing file: " + readResult.message;
                break;
            case kconfig::io::JsonReadError::None:
                *error = "failed to read backing file";
                break;
            }
        }
        return false;
    }
    if (!readResult.json->is_object()) {
        if (error) {
            *error = "backing file must contain a JSON object";
        }
        return false;
    }

    std::lock_guard<std::mutex> lock(internal::g_state.mutex);
    const std::string key(name);
    auto it = internal::g_state.namedConfigs.find(key);
    if (it == internal::g_state.namedConfigs.end()) {
        if (error) {
            *error = "named config not found";
        }
        return false;
    }

    it->second.json = std::move(*readResult.json);
    it->second.revision++;
    if (it->second.isMutable) {
        auto& saveState = internal::g_state.namedSaveStates[key];
        saveState.lastSaveTime = std::chrono::steady_clock::now();
        saveState.lastSavedRevision = it->second.revision;
        saveState.pendingSave = false;
    } else {
        internal::g_state.namedSaveStates.erase(key);
    }
    internal::g_state.revision++;
    return true;
}

bool SetSaveIntervalSeconds(std::string_view name, std::optional<double> seconds) {
    KTRACE("store.requests",
           "SetSaveIntervalSeconds namespace='{}' interval='{}'",
           std::string(name),
           DescribeSaveInterval(seconds));
    if (!IsValidNamespaceName(name)) {
        return false;
    }
    if (seconds.has_value() && (!std::isfinite(*seconds) || *seconds < 0.0)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(internal::g_state.mutex);
    const std::string key(name);
    const auto configIt = internal::g_state.namedConfigs.find(key);
    if (configIt == internal::g_state.namedConfigs.end() || !configIt->second.isMutable) {
        return false;
    }

    auto& saveState = internal::g_state.namedSaveStates[key];
    saveState.saveIntervalSeconds = seconds;
    internal::g_state.revision++;
    return true;
}

std::optional<double> SaveIntervalSeconds(std::string_view name) {
    KTRACE("store.requests",
           "SaveIntervalSeconds namespace='{}'",
           std::string(name));
    if (!IsValidNamespaceName(name)) {
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(internal::g_state.mutex);
    const std::string key(name);
    const auto configIt = internal::g_state.namedConfigs.find(key);
    if (configIt == internal::g_state.namedConfigs.end() || !configIt->second.isMutable) {
        return std::nullopt;
    }

    const auto saveIt = internal::g_state.namedSaveStates.find(key);
    if (saveIt == internal::g_state.namedSaveStates.end()) {
        return std::nullopt;
    }
    return saveIt->second.saveIntervalSeconds;
}

bool FlushPendingWrites(std::string* error) {
    KTRACE("store",
           "FlushPendingWrites requested (enable store.requests for details)");
    KTRACE("store.requests",
           "FlushPendingWrites begin");

    struct PendingWrite {
        std::string name;
        std::filesystem::path path;
        kconfig::json::Value value;
        uint64_t revision = 0;
    };
    std::vector<PendingWrite> pendingWrites;
    const auto now = std::chrono::steady_clock::now();

    {
        std::lock_guard<std::mutex> lock(internal::g_state.mutex);
        for (auto& [name, config] : internal::g_state.namedConfigs) {
            if (!config.isMutable) {
                continue;
            }

            const auto saveIt = internal::g_state.namedSaveStates.find(name);
            if (saveIt == internal::g_state.namedSaveStates.end()) {
                continue;
            }
            auto& saveState = saveIt->second;
            if (saveState.pendingSave && config.revision <= saveState.lastSavedRevision) {
                saveState.pendingSave = false;
            }
            if (!saveState.pendingSave) {
                continue;
            }

            const auto backingIt = internal::g_state.namedBackingFiles.find(name);
            if (backingIt == internal::g_state.namedBackingFiles.end()) {
                KTRACE("store.requests",
                       "FlushPendingWrites namespace='{}' pending=true backing='none' interval='{}' -> skip",
                       name,
                       DescribeSaveInterval(saveState.saveIntervalSeconds));
                continue;
            }

            const double elapsedSeconds = std::chrono::duration<double>(now - saveState.lastSaveTime).count();
            const bool due = !saveState.saveIntervalSeconds.has_value()
                || *saveState.saveIntervalSeconds <= 0.0
                || elapsedSeconds >= *saveState.saveIntervalSeconds;
            KTRACE("store.requests",
                   "FlushPendingWrites namespace='{}' pending=true revision={} last_saved_revision={} interval='{}' elapsed={:.3f}s due={}",
                   name,
                   config.revision,
                   saveState.lastSavedRevision,
                   DescribeSaveInterval(saveState.saveIntervalSeconds),
                   elapsedSeconds,
                   due);
            if (!due) {
                continue;
            }

            pendingWrites.push_back(PendingWrite{
                name,
                backingIt->second,
                config.json,
                config.revision
            });
        }
    }

    KTRACE("store.requests",
           "FlushPendingWrites pending_writes={}",
           pendingWrites.size());
    bool allWritesSucceeded = true;
    for (const auto& pending : pendingWrites) {
        std::string writeError;
        KTRACE("store.requests",
               "FlushPendingWrites namespace='{}' -> WriteJsonFile",
               pending.name);
        if (!writeJsonFileUnlocked(pending.path, pending.value, &writeError)) {
            allWritesSucceeded = false;
            KTRACE("store.requests",
                   "FlushPendingWrites namespace='{}' -> write failed: {}",
                   pending.name,
                   writeError);
            if (error && error->empty()) {
                *error = "FlushPendingWrites failed for '" + pending.name + "': " + writeError;
            }
            continue;
        }

        const auto writeTime = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(internal::g_state.mutex);
        const auto configIt = internal::g_state.namedConfigs.find(pending.name);
        const auto saveIt = internal::g_state.namedSaveStates.find(pending.name);
        const auto backingIt = internal::g_state.namedBackingFiles.find(pending.name);
        if (configIt == internal::g_state.namedConfigs.end()
            || saveIt == internal::g_state.namedSaveStates.end()
            || backingIt == internal::g_state.namedBackingFiles.end()) {
            continue;
        }

        auto& saveState = saveIt->second;
        saveState.lastSaveTime = writeTime;
        saveState.lastSavedRevision = pending.revision;
        saveState.pendingSave = (configIt->second.revision != pending.revision) || (backingIt->second != pending.path);
        KTRACE("store.requests",
               "FlushPendingWrites namespace='{}' -> saved revision={} pending_after={}",
               pending.name,
               pending.revision,
               saveState.pendingSave);
    }

    if (!allWritesSucceeded && error && error->empty()) {
        *error = "FlushPendingWrites failed.";
    }
    return allWritesSucceeded;
}

} // namespace kconfig::store::fs::api

namespace kconfig::store::user::api {

bool SetDirname(std::string_view dirname) {
    KTRACE("store",
           "SetDirname requested: dirname='{}' (enable store.requests for details)",
           std::string(dirname));
    KTRACE("store.requests",
           "SetDirname dirname='{}'",
           std::string(dirname));
    std::filesystem::path dirnamePath(dirname);
    if (dirnamePath.empty()
        || dirnamePath.is_absolute()
        || dirnamePath.has_parent_path()
        || dirnamePath == "."
        || dirnamePath == "..") {
        return false;
    }

    const std::filesystem::path baseDir = DefaultUserConfigDirectory().parent_path();
    if (baseDir.empty()) {
        return false;
    }

    const std::filesystem::path target = kconfig::io::Canonicalize(baseDir / dirnamePath);
    std::lock_guard<std::mutex> lock(internal::g_state.mutex);
    internal::g_state.userConfigDirOverride = target;
    return true;
}

bool OverrideConfigFilePath(const std::filesystem::path& fullFilesystemPath) {
    KTRACE("store",
           "OverrideConfigFilePath requested: path='{}' (enable store.requests for details)",
           fullFilesystemPath.string());
    KTRACE("store.requests",
           "OverrideConfigFilePath path='{}'",
           fullFilesystemPath.string());

    if (fullFilesystemPath.empty()
        || !fullFilesystemPath.has_filename()
        || fullFilesystemPath.filename() == "."
        || fullFilesystemPath.filename() == "..") {
        return false;
    }

    const std::filesystem::path canonical = kconfig::io::Canonicalize(fullFilesystemPath);

    std::lock_guard<std::mutex> lock(internal::g_state.mutex);
    internal::g_state.userConfigFilePathOverride = canonical;
    return true;
}

void ResetConfigLocationOverrides() {
    KTRACE("store.requests", "ResetConfigLocationOverrides");
    std::lock_guard<std::mutex> lock(internal::g_state.mutex);
    internal::g_state.userConfigFilePathOverride.reset();
    internal::g_state.userConfigDirOverride.reset();
}

std::filesystem::path ConfigFilePath() {
    const auto filePath = CurrentUserConfigFilePath();
    KTRACE("store.requests", "ConfigFilePath -> '{}'", filePath.string());
    return filePath;
}

bool HasConfigFile() {
    const std::filesystem::path filePath = CurrentUserConfigFilePath();
    KTRACE("store.requests",
           "HasConfigFile path='{}'",
           filePath.string());
    std::error_code ec;
    return std::filesystem::exists(filePath, ec) && !ec && std::filesystem::is_regular_file(filePath, ec) && !ec;
}

bool InitializeConfigFile(const kconfig::json::Value& json, std::string* error) {
    const std::filesystem::path filePath = CurrentUserConfigFilePath();
    KTRACE("store",
           "InitializeConfigFile requested: path='{}' (enable store.requests for details)",
           filePath.string());
    KTRACE("store.requests",
           "InitializeConfigFile path='{}' json_type='{}'",
           filePath.string(),
           DescribeJsonType(json));
    if (!json.is_object()) {
        if (error) {
            *error = "user config root must be a JSON object";
        }
        return false;
    }
    return kconfig::io::WriteJsonFile(filePath, json, error);
}

bool LoadConfigFile(std::string_view name,
                    const kconfig::store::user::LoadOptions& options,
                    std::string* error) {
    const std::filesystem::path filePath = CurrentUserConfigFilePath();
    KTRACE("store",
           "LoadConfigFile requested: namespace='{}' mode='{}' path='{}' (enable store.requests for details)",
           std::string(name),
           UserLoadModeName(options.mode),
           filePath.string());
    KTRACE("store.requests",
           "LoadConfigFile namespace='{}' mode='{}' path='{}'",
           std::string(name),
           UserLoadModeName(options.mode),
           filePath.string());
    if (!IsValidNamespaceName(name)) {
        if (error) {
            *error = "namespace is invalid";
        }
        return false;
    }

    std::error_code ec;
    if (!std::filesystem::exists(filePath, ec) || ec || !std::filesystem::is_regular_file(filePath, ec) || ec) {
        if (error) {
            *error = "user config file not found: " + filePath.string();
        }
        return false;
    }

    return (options.mode == kconfig::store::user::LoadMode::Mutable)
        ? kconfig::store::fs::api::LoadMutable(name, filePath, error)
        : kconfig::store::fs::api::LoadReadOnly(name, filePath, error);
}

} // namespace kconfig::store::user::api
