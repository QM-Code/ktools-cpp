#pragma once

#include <kcli.hpp>

#include <string>
#include <string_view>

namespace kcli::detail {

bool StartsWith(std::string_view value, std::string_view prefix);
std::string TrimWhitespace(std::string_view value);
bool ContainsWhitespace(std::string_view value);

std::string NormalizeRootNameOrThrow(std::string_view raw_root);
std::string NormalizeInlineRootOptionOrThrow(std::string_view raw_root);
std::string NormalizeInlineHandlerOptionOrThrow(std::string_view raw_option,
                                                std::string_view root_name);
std::string NormalizePrimaryHandlerOptionOrThrow(std::string_view raw_option);
std::string NormalizeAliasOrThrow(std::string_view raw_alias);
std::string NormalizeAliasTargetOptionOrThrow(std::string_view raw_target);
std::string NormalizeHelpPlaceholderOrThrow(std::string_view raw_placeholder);
std::string NormalizeDescriptionOrThrow(std::string_view raw_description);

struct ParseOutcome {
    bool ok = true;
    std::string error_option{};
    std::string error_message{};
};

ParseOutcome MakeError(std::string_view option, std::string_view message);
[[noreturn]] void ThrowCliError(const ParseOutcome& result);

}  // namespace kcli::detail
