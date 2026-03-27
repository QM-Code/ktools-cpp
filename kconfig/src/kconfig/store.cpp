#include <kconfig/store.hpp>

#include "store/api_impl.hpp"

#include <utility>

namespace kconfig::store {

bool Has(std::string_view name) {
    return api::Has(name);
}

bool AddMutable(std::string_view name, const kconfig::json::Value& json) {
    return api::AddMutable(name, json);
}

bool AddReadOnly(std::string_view name, const kconfig::json::Value& json) {
    return api::AddReadOnly(name, json);
}

bool Merge(std::string_view targetName,
           std::initializer_list<std::string_view> sourceNames,
           const MergeOptions& options) {
    return api::Merge(targetName, sourceNames, options);
}

bool Merge(std::string_view targetName,
           const std::vector<std::string>& sourceNames,
           const MergeOptions& options) {
    return api::Merge(targetName, sourceNames, options);
}

bool Unregister(std::string_view name) {
    return api::Unregister(name);
}

std::optional<kconfig::json::Value> Get(std::string_view name, std::string_view path) {
    return api::Get(name, path);
}

bool Set(std::string_view name, std::string_view path, kconfig::json::Value value) {
    return api::Set(name, path, std::move(value));
}

bool Erase(std::string_view name, std::string_view path) {
    return api::Erase(name, path);
}

bool SetMutable(std::string_view name) {
    return api::SetMutable(name);
}

bool SetReadOnly(std::string_view name) {
    return api::SetReadOnly(name);
}

bool IsMutable(std::string_view name) {
    return api::IsMutable(name);
}

} // namespace kconfig::store
