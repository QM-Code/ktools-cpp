#pragma once

#include <kcli.hpp>

#include <string>
#include <string_view>

namespace kconfig::cli {

bool StoreAssignment(std::string_view name,
                     std::string_view text,
                     std::string* error = nullptr);

kcli::InlineParser GetInlineParser(std::string_view config_root = "config");

} // namespace kconfig::cli
