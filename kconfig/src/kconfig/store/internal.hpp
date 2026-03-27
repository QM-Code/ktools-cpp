#pragma once

#include "../store.hpp"

#include <chrono>
#include <filesystem>
#include <map>
#include <mutex>
#include <optional>
#include <string>

namespace kconfig::store::internal {

struct SaveState {
    std::optional<double> saveIntervalSeconds{};
    std::chrono::steady_clock::time_point lastSaveTime = std::chrono::steady_clock::now();
    uint64_t lastSavedRevision = 0;
    bool pendingSave = false;
};

struct ConfigStoreState {
    std::mutex mutex;
    uint64_t revision = 0;
    std::map<std::string, kconfig::store::ConfigLayer> namedConfigs;
    std::map<std::string, std::filesystem::path> namedBackingFiles;
    std::map<std::string, std::filesystem::path> namedAssetRoots;
    std::map<std::string, SaveState> namedSaveStates;
    std::optional<std::filesystem::path> userConfigFilePathOverride;
    std::optional<std::filesystem::path> userConfigDirOverride;
};

extern ConfigStoreState g_state;

} // namespace kconfig::store::internal
