#include <kconfig/cli.hpp>

#include "kconfig/trace.hpp"
#include "store.hpp"

#include <kconfig/store.hpp>
#include <kconfig/store/user.hpp>

#include <kcli.hpp>

#include <cctype>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace {

std::string trimWhitespace(std::string_view value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
        value.remove_prefix(1);
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
        value.remove_suffix(1);
    }
    return std::string(value);
}

bool isQuoted(std::string_view value) {
    if (value.size() < 2) {
        return false;
    }
    const char quote = value.front();
    return (quote == '"' || quote == '\'') && value.back() == quote;
}

std::string unquote(std::string_view value) {
    if (!isQuoted(value)) {
        return std::string(value);
    }

    std::string result;
    result.reserve(value.size() - 2);
    bool escaped = false;
    for (std::size_t i = 1; i + 1 < value.size(); ++i) {
        const char ch = value[i];
        if (escaped) {
            result.push_back(ch);
            escaped = false;
            continue;
        }
        if (ch == '\\') {
            escaped = true;
            continue;
        }
        result.push_back(ch);
    }
    if (escaped) {
        result.push_back('\\');
    }
    return result;
}

std::size_t findUnquotedChar(std::string_view value, char target) {
    bool inSingleQuote = false;
    bool inDoubleQuote = false;
    bool escaped = false;

    for (std::size_t i = 0; i < value.size(); ++i) {
        const char ch = value[i];
        if (escaped) {
            escaped = false;
            continue;
        }
        if ((inSingleQuote || inDoubleQuote) && ch == '\\') {
            escaped = true;
            continue;
        }
        if (!inDoubleQuote && ch == '\'') {
            inSingleQuote = !inSingleQuote;
            continue;
        }
        if (!inSingleQuote && ch == '"') {
            inDoubleQuote = !inDoubleQuote;
            continue;
        }
        if (!inSingleQuote && !inDoubleQuote && ch == target) {
            return i;
        }
    }

    return std::string_view::npos;
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

bool parseCliConfigValue(std::string_view rawValue, kconfig::json::Value& outValue, std::string* error) {
    const std::string text = trimWhitespace(rawValue);
    if (text.empty()) {
        if (error) {
            *error = "config assignment value must not be empty";
        }
        return false;
    }

    if (isQuoted(text) && text.front() == '\'') {
        outValue = kconfig::json::Value(unquote(text));
        return true;
    }

    try {
        outValue = kconfig::json::Parse(text);
        return true;
    } catch (...) {
        outValue = isQuoted(text) ? kconfig::json::Value(unquote(text)) : kconfig::json::Value(text);
        return true;
    }
}

bool parseCliAssignment(std::string_view text,
                        std::string& outPath,
                        kconfig::json::Value& outValue,
                        std::string* error) {
    const std::string expression = trimWhitespace(text);
    if (expression.empty()) {
        if (error) {
            *error = "config assignment must not be empty";
        }
        return false;
    }

    const std::size_t equal = findUnquotedChar(expression, '=');
    if (equal == std::string_view::npos) {
        if (error) {
            *error = "config assignment must be '\"<key>\"=<value>'";
        }
        return false;
    }

    const std::string left = trimWhitespace(expression.substr(0, equal));
    const std::string right = trimWhitespace(expression.substr(equal + 1));
    if (left.empty() || right.empty()) {
        if (error) {
            *error = "config assignment must include both key and value";
        }
        return false;
    }
    if (!isQuoted(left)) {
        if (error) {
            *error = "config assignment key must be quoted: '\"<key>\"=<value>'";
        }
        return false;
    }

    outPath = trimWhitespace(unquote(left));
    if (outPath.empty()) {
        if (error) {
            *error = "config assignment key must not be empty";
        }
        return false;
    }

    if (!parseCliConfigValue(right, outValue, error)) {
        return false;
    }

    kconfig::json::Value probe = kconfig::json::Object();
    if (!kconfig::store::setValueAtPath(probe, outPath, outValue)) {
        if (error) {
            *error = "config assignment key path is invalid";
        }
        return false;
    }

    return true;
}

void applyAssignmentOrThrow(const std::string_view option,
                            std::string_view configNamespace,
                            std::string_view rawAssignment) {
    std::string error;
    if (!kconfig::cli::StoreAssignment(configNamespace, rawAssignment, &error)) {
        throw std::runtime_error("failed to store CLI config from option '"
                                 + std::string(option)
                                 + "': "
                                 + error);
    }

    KTRACE("cli",
           "option '{}' stored assignment in namespace '{}': {}",
           option,
           std::string(configNamespace),
           std::string(rawAssignment));
}

void applyUserConfigPathOrThrow(const std::string_view option, std::string_view rawPath) {
    const std::string trimmedPath = trimWhitespace(rawPath);
    if (trimmedPath.empty()) {
        throw std::invalid_argument("user config path must not be empty");
    }

    if (!kconfig::store::user::OverrideConfigFilePath(std::filesystem::path(trimmedPath))) {
        throw std::runtime_error("failed to set user config path from option '" + std::string(option) + "'");
    }

    KTRACE("cli",
           "option '{}' set user config path '{}'",
           option,
           std::filesystem::path(trimmedPath).string());
}

void printConfigExamples(const std::string& root) {
    std::cout
        << "\nKConfig examples:\n"
        << "  " << root << " '\"client.Language\"=\"en\"'\n"
        << "  " << root << " '\"graphics.width\"=1920'\n"
        << "  " << root << " '\"graphics.fullscreen\"=true'\n"
        << "  " << root << " '\"audio.devices[0]\"=\"default\"'\n\n";
}

void _ROOT(const kcli::HandlerContext& context, std::string_view value) {
    applyAssignmentOrThrow(context.option, context.root, value);
}

void _examples(const kcli::HandlerContext& context) {
    printConfigExamples("--" + std::string(context.root));
}

void _user(const kcli::HandlerContext& context, std::string_view value) {
    applyUserConfigPathOrThrow(context.option, value);
}

} // namespace

namespace kconfig::cli {

bool StoreAssignment(std::string_view name,
                     std::string_view text,
                     std::string* error) {
    KTRACE("cli",
           "StoreAssignment requested: namespace='{}' text='{}'",
           std::string(name),
           std::string(text));
    if (!IsValidNamespaceName(name)) {
        if (error) {
            *error = "namespace is invalid";
        }
        return false;
    }

    std::string path;
    kconfig::json::Value value;
    if (!parseCliAssignment(text, path, value, error)) {
        return false;
    }

    if (!kconfig::store::Has(name)) {
        if (!kconfig::store::AddMutable(name, kconfig::json::Object())) {
            if (error) {
                *error = "failed to create mutable config namespace '" + std::string(name) + "'";
            }
            return false;
        }
    }

    if (!kconfig::store::Set(name, path, std::move(value))) {
        if (error) {
            *error = "failed to set '" + std::string(name) + "." + path + "'";
        }
        return false;
    }

    KTRACE("cli",
           "StoreAssignment namespace='{}' key='{}' -> stored",
           std::string(name),
           path);
    return true;
}

kcli::InlineParser GetInlineParser(std::string_view config_root) {
    kcli::InlineParser parser("config");
    if (!config_root.empty()) {
        parser.setRoot(config_root);
    }
    parser.setRootValueHandler(_ROOT,
                               "<assignment>",
                               "Store a config assignment in the target namespace.");
    parser.setHandler("-examples", _examples, "Show assignment examples.");
    parser.setHandler("-user",
                      _user,
                      "Override user config file path for store::user::LoadConfigFile.");
    return parser;
}

} // namespace kconfig::cli
