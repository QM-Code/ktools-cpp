#pragma once

#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <kconfig/json.hpp>
#include <kconfig/store/fs.hpp>
#include <kconfig/store/read.hpp>
#include <kconfig/store/user.hpp>

namespace kconfig::store {

enum class Mode {
    ReadOnly,
    Mutable
};

struct MergeOptions {
    Mode mode = Mode::Mutable;
};

bool Has(std::string_view name);

bool AddMutable(std::string_view name, const kconfig::json::Value& json);
bool AddReadOnly(std::string_view name, const kconfig::json::Value& json);

bool Merge(std::string_view targetName,
           std::initializer_list<std::string_view> sourceNames,
           const MergeOptions& options = {});

bool Merge(std::string_view targetName,
           const std::vector<std::string>& sourceNames,
           const MergeOptions& options = {});

bool Unregister(std::string_view name);

std::optional<kconfig::json::Value> Get(std::string_view name,
                                        std::string_view path = ".");

bool Set(std::string_view name,
         std::string_view path,
         kconfig::json::Value value);

bool Erase(std::string_view name,
           std::string_view path);

bool SetMutable(std::string_view name);
bool SetReadOnly(std::string_view name);
bool IsMutable(std::string_view name);

} // namespace kconfig::store
