#include <kconfig.hpp>
#include <kconfig/asset.hpp>

#include "kconfig/trace.hpp"
#include "store.hpp"
#include "store/internal.hpp"
#include "io.hpp"

#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace kconfig::asset {
namespace {

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

std::optional<std::filesystem::path> ResolvePathInternal(std::string_view name,
                                                         std::string_view jsonPath,
                                                         std::string* error) {
    KTRACE("asset",
           "ResolvePath requested: namespace='{}' path='{}' (enable asset.requests for details)",
           std::string(name),
           std::string(jsonPath));
    KTRACE("asset.requests",
           "ResolvePath namespace='{}' path='{}'",
           std::string(name),
           std::string(jsonPath));
    if (!IsValidNamespaceName(name)) {
        if (error) {
            *error = "namespace is invalid";
        }
        return std::nullopt;
    }
    if (jsonPath.empty()) {
        if (error) {
            *error = "json path must not be empty";
        }
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(kconfig::store::internal::g_state.mutex);
    const auto it = kconfig::store::internal::g_state.namedConfigs.find(std::string(name));
    if (it == kconfig::store::internal::g_state.namedConfigs.end()) {
        if (error) {
            *error = "namespace not found";
        }
        return std::nullopt;
    }

    const kconfig::json::Value* node = kconfig::store::isRootPath(jsonPath)
        ? &it->second.json
        : kconfig::store::resolvePath(it->second.json, jsonPath);
    if (node == nullptr) {
        if (error) {
            *error = "json path not found";
        }
        return std::nullopt;
    }
    if (!node->is_string()) {
        if (error) {
            *error = "asset value must be a string path";
        }
        return std::nullopt;
    }

    std::filesystem::path candidate(node->get<std::string>());
    if (candidate.empty()) {
        if (error) {
            *error = "asset path must not be empty";
        }
        return std::nullopt;
    }
    if (candidate.is_absolute()) {
        return kconfig::io::Canonicalize(candidate);
    }

    std::filesystem::path baseDir;
    if (const auto assetRootIt = kconfig::store::internal::g_state.namedAssetRoots.find(std::string(name));
        assetRootIt != kconfig::store::internal::g_state.namedAssetRoots.end()) {
        baseDir = assetRootIt->second;
    } else if (const auto backingIt = kconfig::store::internal::g_state.namedBackingFiles.find(std::string(name));
               backingIt != kconfig::store::internal::g_state.namedBackingFiles.end()) {
        baseDir = backingIt->second.parent_path();
    } else {
        baseDir = it->second.baseDir;
    }

    if (baseDir.empty()) {
        if (error) {
            *error = "relative asset path requires an asset root or backing file";
        }
        return std::nullopt;
    }

    return kconfig::io::Canonicalize(baseDir / candidate);
}

std::vector<std::uint8_t> ReadBytesOrThrow(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("failed to open asset: " + path.string());
    }
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

std::string ReadTextOrThrow(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("failed to open asset: " + path.string());
    }
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

} // namespace

std::optional<std::filesystem::path> ResolvePath(std::string_view name,
                                                 std::string_view jsonPath) {
    std::string error;
    const auto path = ResolvePathInternal(name, jsonPath, &error);
    if (!path.has_value()) {
        const auto log = kconfig::GetTraceLogger();
        log.warn("asset::ResolvePath({}, {}) failed: {}",
                 std::string(name),
                 std::string(jsonPath),
                 error);
    }
    return path;
}

std::filesystem::path RequiredPath(std::string_view name,
                                   std::string_view jsonPath) {
    std::string error;
    const auto path = ResolvePathInternal(name, jsonPath, &error);
    if (!path.has_value()) {
        throw std::runtime_error("Missing required asset path "
                                 + std::string(name)
                                 + "."
                                 + std::string(jsonPath)
                                 + ": "
                                 + error);
    }
    return *path;
}

bool Exists(std::string_view name,
            std::string_view jsonPath) {
    const auto path = ResolvePath(name, jsonPath);
    if (!path.has_value()) {
        return false;
    }
    std::error_code ec;
    return std::filesystem::exists(*path, ec) && !ec && std::filesystem::is_regular_file(*path, ec) && !ec;
}

std::vector<std::uint8_t> LoadBytes(std::string_view name,
                                    std::string_view jsonPath,
                                    std::vector<std::uint8_t> defaultValue) {
    try {
        const auto path = RequiredPath(name, jsonPath);
        return ReadBytesOrThrow(path);
    } catch (const std::exception& ex) {
        const auto log = kconfig::GetTraceLogger();
        log.warn("asset::LoadBytes({}, {}) failed: {}",
                 std::string(name),
                 std::string(jsonPath),
                 ex.what());
        return defaultValue;
    }
}

std::vector<std::uint8_t> LoadRequiredBytes(std::string_view name,
                                            std::string_view jsonPath) {
    return ReadBytesOrThrow(RequiredPath(name, jsonPath));
}

std::string LoadText(std::string_view name,
                     std::string_view jsonPath,
                     std::string defaultValue) {
    try {
        const auto path = RequiredPath(name, jsonPath);
        return ReadTextOrThrow(path);
    } catch (const std::exception& ex) {
        const auto log = kconfig::GetTraceLogger();
        log.warn("asset::LoadText({}, {}) failed: {}",
                 std::string(name),
                 std::string(jsonPath),
                 ex.what());
        return defaultValue;
    }
}

std::string LoadRequiredText(std::string_view name,
                             std::string_view jsonPath) {
    return ReadTextOrThrow(RequiredPath(name, jsonPath));
}

kconfig::json::Value LoadJson(std::string_view name,
                              std::string_view jsonPath,
                              kconfig::json::Value defaultValue) {
    try {
        const auto path = RequiredPath(name, jsonPath);
        const auto readResult = kconfig::io::ReadJsonFile(path);
        if (!readResult.json.has_value()) {
            throw std::runtime_error(readResult.message.empty() ? "failed to read JSON asset" : readResult.message);
        }
        return *readResult.json;
    } catch (const std::exception& ex) {
        const auto log = kconfig::GetTraceLogger();
        log.warn("asset::LoadJson({}, {}) failed: {}",
                 std::string(name),
                 std::string(jsonPath),
                 ex.what());
        return defaultValue;
    }
}

kconfig::json::Value LoadRequiredJson(std::string_view name,
                                      std::string_view jsonPath) {
    const auto path = RequiredPath(name, jsonPath);
    const auto readResult = kconfig::io::ReadJsonFile(path);
    if (!readResult.json.has_value()) {
        throw std::runtime_error("failed to load JSON asset '" + path.string() + "': " + readResult.message);
    }
    return *readResult.json;
}

} // namespace kconfig::asset
