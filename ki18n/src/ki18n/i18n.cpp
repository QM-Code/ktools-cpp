#include <ki18n.hpp>

#include <kconfig/asset.hpp>
#include <kconfig/json.hpp>
#include <kconfig/store.hpp>

#include <ktrace.hpp>

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace ki18n {
namespace {

enum class MissingLogLevel {
    Warning,
    Error
};

struct State {
    std::string language = "en";
    std::unordered_map<std::string, std::string> fallbackStrings;
    std::unordered_map<std::string, std::string> selectedStrings;
    std::unordered_map<std::string, std::string> missingCache;
};

State& GetState() {
    static State state;
    return state;
}

std::string normalizeLanguage(std::string value) {
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    value.erase(std::find_if(value.rbegin(), value.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), value.end());
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string Qualified(std::string_view configNamespace, std::string_view path) {
    std::string qualified(configNamespace);
    qualified.push_back('.');
    qualified.append(path.begin(), path.end());
    return qualified;
}

void flattenStrings(const kconfig::json::Value& node,
                    const std::string& prefix,
                    std::unordered_map<std::string, std::string>& out) {
    if (node.is_string()) {
        out[prefix] = node.get<std::string>();
        return;
    }
    if (!node.is_object()) {
        return;
    }
    for (auto it = node.begin(); it != node.end(); ++it) {
        const std::string key = it.key();
        if (key.empty()) {
            continue;
        }
        const std::string next = prefix.empty() ? key : (prefix + "." + key);
        flattenStrings(it.value(), next, out);
    }
}

std::unordered_map<std::string, std::string> loadLanguageStrings(std::string_view configNamespace,
                                                                 std::string_view language,
                                                                 MissingLogLevel missingLevel) {
    const auto log = ki18n::GetTraceLogger();
    std::unordered_map<std::string, std::string> result;
    try {
        const kconfig::json::Value root = kconfig::asset::LoadRequiredJson(configNamespace,
                                                                           std::string("strings.") + std::string(language));
        if (!root.is_object()) {
            if (missingLevel == MissingLogLevel::Error) {
                log.error("i18n: language asset for '{}' is not a JSON object.", std::string(language));
            } else {
                log.warn("i18n: language asset for '{}' is not a JSON object.", std::string(language));
            }
            return result;
        }

        const auto stringsIt = root.find("strings");
        if (stringsIt != root.end() && stringsIt->is_object()) {
            flattenStrings(*stringsIt, "", result);
            return result;
        }

        flattenStrings(root, "", result);
        return result;
    } catch (const std::exception& ex) {
        if (missingLevel == MissingLogLevel::Error) {
            log.error("i18n: failed to load language '{}' from namespace '{}': {}",
                      std::string(language),
                      std::string(configNamespace),
                      ex.what());
        } else {
            log.warn("i18n: failed to load language '{}' from namespace '{}': {}",
                     std::string(language),
                     std::string(configNamespace),
                     ex.what());
        }
        return result;
    }
}

} // namespace

void Reset() {
    auto& state = GetState();
    state.language = "en";
    state.fallbackStrings.clear();
    state.selectedStrings.clear();
    state.missingCache.clear();
}

void LoadFromConfig(std::string_view configNamespace) {
    std::string language = kconfig::store::read::NonEmptyString(Qualified(configNamespace, "defaultLanguage"), "en");
    language = normalizeLanguage(std::move(language));
    if (language.empty()) {
        language = "en";
    }
    LoadLanguage(configNamespace, language);
}

void LoadLanguage(std::string_view configNamespace, std::string_view language) {
    auto& state = GetState();
    const auto log = ki18n::GetTraceLogger();
    state.missingCache.clear();

    std::string fallbackLanguage = kconfig::store::read::NonEmptyString(Qualified(configNamespace, "fallbackLanguage"), "en");
    fallbackLanguage = normalizeLanguage(std::move(fallbackLanguage));
    if (fallbackLanguage.empty()) {
        fallbackLanguage = "en";
    }

    state.fallbackStrings = loadLanguageStrings(configNamespace, fallbackLanguage, MissingLogLevel::Error);
    state.selectedStrings.clear();

    std::string selectedLanguage = normalizeLanguage(std::string(language));
    if (selectedLanguage.empty()) {
        selectedLanguage = fallbackLanguage;
    }

    if (selectedLanguage != fallbackLanguage) {
        state.selectedStrings = loadLanguageStrings(configNamespace, selectedLanguage, MissingLogLevel::Warning);
        if (state.selectedStrings.empty()) {
            log.warn("i18n: falling back to '{}'; language '{}' not found or empty.",
                     fallbackLanguage,
                     selectedLanguage);
            selectedLanguage = fallbackLanguage;
        }
    }

    state.language = selectedLanguage;
}

const std::string& Get(std::string_view key) {
    auto& state = GetState();
    const std::string keyString(key);
    if (auto it = state.selectedStrings.find(keyString); it != state.selectedStrings.end()) {
        return it->second;
    }
    if (auto it = state.fallbackStrings.find(keyString); it != state.fallbackStrings.end()) {
        return it->second;
    }
    auto [it, inserted] = state.missingCache.emplace(keyString, keyString);
    return it->second;
}

const std::string& Require(std::string_view key) {
    auto& state = GetState();
    const std::string keyString(key);
    if (auto it = state.selectedStrings.find(keyString); it != state.selectedStrings.end()) {
        return it->second;
    }
    if (auto it = state.fallbackStrings.find(keyString); it != state.fallbackStrings.end()) {
        return it->second;
    }
    throw std::runtime_error("Missing required i18n key: " + keyString);
}

std::string Format(std::string_view key,
                   std::initializer_list<std::pair<std::string, std::string>> replacements) {
    return FormatText(Get(key), replacements);
}

std::string FormatText(std::string_view text,
                       std::initializer_list<std::pair<std::string, std::string>> replacements) {
    std::string result(text);
    for (const auto& pair : replacements) {
        const std::string token = "{" + pair.first + "}";
        std::size_t pos = 0;
        while ((pos = result.find(token, pos)) != std::string::npos) {
            result.replace(pos, token.size(), pair.second);
            pos += pair.second.size();
        }
    }
    return result;
}

const std::string& Language() {
    return GetState().language;
}

} // namespace ki18n
