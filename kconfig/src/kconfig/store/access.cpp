#include "../store.hpp"

#include "kconfig/trace.hpp"
#include "api_impl.hpp"
#include "internal.hpp"

#include <mutex>
#include <optional>
#include <string>
#include <string_view>

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

} // namespace

namespace kconfig::store {

using internal::g_state;

const kconfig::json::Value* findNamedValueLocked(std::string_view name) {
    const auto it = g_state.namedConfigs.find(std::string(name));
    if (it == g_state.namedConfigs.end()) {
        return nullptr;
    }
    return &it->second.json;
}

kconfig::json::Value* findMutableNamedValueLocked(std::string_view name) {
    const auto it = g_state.namedConfigs.find(std::string(name));
    if (it == g_state.namedConfigs.end() || !it->second.isMutable) {
        return nullptr;
    }
    return &it->second.json;
}

} // namespace kconfig::store

namespace kconfig::store::api {

using internal::g_state;

std::optional<kconfig::json::Value> Get(std::string_view name,
                                        std::string_view path) {
    KTRACE("store.requests",
           "Get namespace='{}' path='{}'",
           std::string(name),
           std::string(path));
    std::lock_guard<std::mutex> lock(g_state.mutex);
    const auto* root = kconfig::store::findNamedValueLocked(name);
    if (!root) {
        KTRACE("store.requests",
               "Get namespace='{}' path='{}' -> miss (namespace not found)",
               std::string(name),
               std::string(path));
        return std::nullopt;
    }
    if (kconfig::store::isRootPath(path)) {
        KTRACE("store.requests",
               "Get namespace='{}' path='{}' -> hit root type='{}'",
               std::string(name),
               std::string(path),
               DescribeJsonType(*root));
        return std::optional<kconfig::json::Value>(std::in_place, *root);
    }
    if (const auto* resolved = kconfig::store::resolvePath(*root, path)) {
        KTRACE("store.requests",
               "Get namespace='{}' path='{}' -> hit type='{}'",
               std::string(name),
               std::string(path),
               DescribeJsonType(*resolved));
        return std::optional<kconfig::json::Value>(std::in_place, *resolved);
    }
    KTRACE("store.requests",
           "Get namespace='{}' path='{}' -> miss (path not found)",
           std::string(name),
           std::string(path));
    return std::nullopt;
}

bool Set(std::string_view name,
         std::string_view path,
         kconfig::json::Value value) {
    KTRACE("store.requests",
           "Set namespace='{}' path='{}' value_type='{}'",
           std::string(name),
           std::string(path),
           DescribeJsonType(value));
    if (name.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_state.mutex);
    const std::string key(name);
    auto configIt = g_state.namedConfigs.find(key);
    if (configIt == g_state.namedConfigs.end() || !configIt->second.isMutable) {
        return false;
    }
    auto& target = configIt->second.json;

    if (kconfig::store::isRootPath(path)) {
        target = std::move(value);
    } else if (!kconfig::store::setValueAtPath(target, path, std::move(value))) {
        return false;
    }

    configIt->second.revision++;
    auto& saveState = g_state.namedSaveStates[key];
    saveState.pendingSave = (g_state.namedBackingFiles.find(key) != g_state.namedBackingFiles.end());
    g_state.revision++;
    return true;
}

bool Erase(std::string_view name,
           std::string_view path) {
    KTRACE("store.requests",
           "Erase namespace='{}' path='{}'",
           std::string(name),
           std::string(path));
    if (name.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_state.mutex);
    const std::string key(name);
    auto configIt = g_state.namedConfigs.find(key);
    if (configIt == g_state.namedConfigs.end() || !configIt->second.isMutable) {
        return false;
    }
    auto& target = configIt->second.json;

    const bool changed = kconfig::store::isRootPath(path)
        ? ((target = kconfig::json::Object()), true)
        : kconfig::store::eraseValueAtPath(target, path);
    if (!changed) {
        return false;
    }

    configIt->second.revision++;
    auto& saveState = g_state.namedSaveStates[key];
    saveState.pendingSave = (g_state.namedBackingFiles.find(key) != g_state.namedBackingFiles.end());
    g_state.revision++;
    return true;
}

} // namespace kconfig::store::api
