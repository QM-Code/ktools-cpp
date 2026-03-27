#pragma once

#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>

namespace ki18n {

void Reset();
void LoadFromConfig(std::string_view configNamespace);
void LoadLanguage(std::string_view configNamespace, std::string_view language);

const std::string& Get(std::string_view key);
const std::string& Require(std::string_view key);
std::string Format(std::string_view key,
                   std::initializer_list<std::pair<std::string, std::string>> replacements);
std::string FormatText(std::string_view text,
                       std::initializer_list<std::pair<std::string, std::string>> replacements);

const std::string& Language();

} // namespace ki18n
