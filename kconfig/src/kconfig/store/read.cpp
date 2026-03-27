#include <kconfig.hpp>
#include <kconfig/store.hpp>
#include <kconfig/store/read.hpp>

#include <ktrace.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace kconfig::store::read {

namespace {

struct NamespacedPath {
    std::string name;
    std::string path;
};

bool ParseNamespacedPath(std::string_view qualifiedPath,
                         NamespacedPath& parsed,
                         std::string* error = nullptr) {
    if (qualifiedPath.empty()) {
        if (error) {
            *error = "path must not be empty";
        }
        return false;
    }

    const std::size_t dot = qualifiedPath.find('.');
    if (dot == std::string_view::npos || dot == 0) {
        if (error) {
            *error = "expected '<namespace>.<json-path>'";
        }
        return false;
    }

    parsed.name = std::string(qualifiedPath.substr(0, dot));
    for (const char ch : parsed.name) {
        if (std::isspace(static_cast<unsigned char>(ch)) != 0) {
            if (error) {
                *error = "namespace must not contain whitespace";
            }
            return false;
        }
    }

    const auto suffix = qualifiedPath.substr(dot + 1);
    parsed.path = suffix.empty() ? std::string(".") : std::string(suffix);
    return true;
}

std::optional<kconfig::json::Value> GetNamespacedValue(std::string_view qualifiedPath,
                                                        bool warnOnInvalidPath = true) {
    NamespacedPath parsed;
    std::string parseError;
    if (!ParseNamespacedPath(qualifiedPath, parsed, &parseError)) {
        if (warnOnInvalidPath) {
            const auto log = kconfig::GetTraceLogger();
            log.warn("Config path '{}' is invalid: {}", std::string(qualifiedPath), parseError);
        }
        return std::nullopt;
    }

    return kconfig::store::Get(parsed.name, parsed.path);
}

NamespacedPath ParseRequiredNamespacedPath(std::string_view qualifiedPath) {
    NamespacedPath parsed;
    std::string parseError;
    if (!ParseNamespacedPath(qualifiedPath, parsed, &parseError)) {
        throw std::runtime_error(std::string("Invalid required config path: ")
                                 + std::string(qualifiedPath)
                                 + " ("
                                 + parseError
                                 + ")");
    }
    return parsed;
}

std::string ToLower(std::string value) {
    std::transform(value.begin(),
                   value.end(),
                   value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

std::optional<bool> TryParseBoolValue(const kconfig::json::Value& value) {
    if (value.is_boolean()) {
        return value.get<bool>();
    }
    if (value.is_number_integer()) {
        return value.get<long long>() != 0;
    }
    if (value.is_number_float()) {
        return value.get<double>() != 0.0;
    }
    if (value.is_string()) {
        const std::string text = ToLower(value.get<std::string>());
        if (text == "true" || text == "1" || text == "yes" || text == "on") {
            return true;
        }
        if (text == "false" || text == "0" || text == "no" || text == "off") {
            return false;
        }
    }
    return std::nullopt;
}

std::optional<unsigned long long> TryParseUnsignedValue(const kconfig::json::Value& value) {
    if (value.is_number_unsigned()) {
        return value.get<unsigned long long>();
    }
    if (value.is_number_integer()) {
        const long long signedValue = value.get<long long>();
        if (signedValue < 0) {
            return std::nullopt;
        }
        return static_cast<unsigned long long>(signedValue);
    }
    if (value.is_string()) {
        const std::string raw = value.get<std::string>();
        try {
            size_t consumed = 0;
            const unsigned long long parsed = std::stoull(raw, &consumed, 0);
            if (consumed == raw.size()) {
                return parsed;
            }
        } catch (...) {
        }
    }
    return std::nullopt;
}

std::optional<uint16_t> TryParseUInt16Value(const kconfig::json::Value& value) {
    if (const auto parsed = TryParseUnsignedValue(value)) {
        if (*parsed <= std::numeric_limits<uint16_t>::max()) {
            return static_cast<uint16_t>(*parsed);
        }
    }
    if (value.is_number_float()) {
        const double parsed = value.get<double>();
        if (std::isfinite(parsed)
            && parsed >= 0.0
            && parsed <= static_cast<double>(std::numeric_limits<uint16_t>::max())) {
            return static_cast<uint16_t>(parsed);
        }
    }
    return std::nullopt;
}

std::optional<uint32_t> TryParseUInt32Value(const kconfig::json::Value& value) {
    if (const auto parsed = TryParseUnsignedValue(value)) {
        if (*parsed <= std::numeric_limits<uint32_t>::max()) {
            return static_cast<uint32_t>(*parsed);
        }
    }
    return std::nullopt;
}

std::optional<float> TryParseFloatValue(const kconfig::json::Value& value) {
    if (value.is_number_float()) {
        return static_cast<float>(value.get<double>());
    }
    if (value.is_number_integer()) {
        return static_cast<float>(value.get<long long>());
    }
    if (value.is_string()) {
        try {
            return std::stof(value.get<std::string>());
        } catch (...) {
        }
    }
    return std::nullopt;
}

std::optional<std::vector<float>> TryParseFloatArrayValue(const kconfig::json::Value& value) {
    if (!value.is_array()) {
        return std::nullopt;
    }

    std::vector<float> out;
    out.reserve(value.size());
    for (const auto& entry : value) {
        if (entry.is_number_float()) {
            out.push_back(static_cast<float>(entry.get<double>()));
            continue;
        }
        if (entry.is_number_integer()) {
            out.push_back(static_cast<float>(entry.get<long long>()));
            continue;
        }
        return std::nullopt;
    }
    return out;
}

std::optional<std::vector<std::string>> TryParseStringArrayValue(const kconfig::json::Value& value) {
    if (!value.is_array()) {
        return std::nullopt;
    }

    std::vector<std::string> out;
    out.reserve(value.size());
    for (const auto& entry : value) {
        if (!entry.is_string()) {
            return std::nullopt;
        }
        out.push_back(entry.get<std::string>());
    }
    return out;
}

} // namespace

bool Bool(std::initializer_list<const char*> paths, bool defaultValue) {
    const auto log = kconfig::GetTraceLogger();
    for (const char* path : paths) {
        if (path == nullptr || *path == '\0') {
            continue;
        }
        if (const auto value = GetNamespacedValue(path); value) {
            if (const auto parsed = TryParseBoolValue(*value)) {
                return *parsed;
            }
            log.warn("Config '{}' cannot be interpreted as boolean", path);
        }
    }
    return defaultValue;
}

bool RequiredBool(std::string_view path) {
    if (path.empty()) {
        throw std::runtime_error("Missing required boolean config path");
    }
    const auto parsedPath = ParseRequiredNamespacedPath(path);
    if (const auto value = kconfig::store::Get(parsedPath.name, parsedPath.path); value) {
        if (const auto parsed = TryParseBoolValue(*value)) {
            return *parsed;
        }
        throw std::runtime_error(std::string("Invalid required boolean config: ") + std::string(path));
    }
    throw std::runtime_error(std::string("Missing required boolean config: ") + std::string(path));
}

uint16_t Uint16(std::initializer_list<const char*> paths, uint16_t defaultValue) {
    const auto log = kconfig::GetTraceLogger();
    for (const char* path : paths) {
        if (path == nullptr || *path == '\0') {
            continue;
        }
        if (const auto value = GetNamespacedValue(path); value) {
            if (const auto parsed = TryParseUInt16Value(*value)) {
                return *parsed;
            }
            log.warn("Config '{}' cannot be interpreted as uint16", path);
        }
    }
    return defaultValue;
}

uint16_t RequiredUint16(std::string_view path) {
    if (path.empty()) {
        throw std::runtime_error("Missing required uint16 config path");
    }
    const auto parsedPath = ParseRequiredNamespacedPath(path);
    if (const auto value = kconfig::store::Get(parsedPath.name, parsedPath.path); value) {
        if (const auto parsed = TryParseUInt16Value(*value)) {
            return *parsed;
        }
        throw std::runtime_error(std::string("Invalid required uint16 config: ") + std::string(path));
    }
    throw std::runtime_error(std::string("Missing required uint16 config: ") + std::string(path));
}

uint16_t PositiveUint16(std::initializer_list<const char*> paths, uint16_t defaultValue) {
    const uint16_t value = Uint16(paths, defaultValue);
    if (value == 0) {
        return defaultValue;
    }
    return value;
}

uint16_t RequiredPositiveUint16(std::string_view path) {
    const uint16_t value = RequiredUint16(path);
    if (value == 0) {
        throw std::runtime_error(std::string("Invalid required uint16 config: ")
                                 + std::string(path)
                                 + " (must be > 0)");
    }
    return value;
}

uint32_t Uint32(std::initializer_list<const char*> paths, uint32_t defaultValue) {
    const auto log = kconfig::GetTraceLogger();
    for (const char* path : paths) {
        if (path == nullptr || *path == '\0') {
            continue;
        }
        if (const auto value = GetNamespacedValue(path); value) {
            if (const auto parsed = TryParseUInt32Value(*value)) {
                return *parsed;
            }
            log.warn("Config '{}' cannot be interpreted as uint32", path);
        }
    }
    return defaultValue;
}

uint32_t RequiredUint32(std::string_view path) {
    if (path.empty()) {
        throw std::runtime_error("Missing required uint32 config path");
    }
    const auto parsedPath = ParseRequiredNamespacedPath(path);
    if (const auto value = kconfig::store::Get(parsedPath.name, parsedPath.path); value) {
        if (const auto parsed = TryParseUInt32Value(*value)) {
            return *parsed;
        }
        throw std::runtime_error(std::string("Invalid required uint32 config: ") + std::string(path));
    }
    throw std::runtime_error(std::string("Missing required uint32 config: ") + std::string(path));
}

float Float(std::initializer_list<const char*> paths, float defaultValue) {
    const auto log = kconfig::GetTraceLogger();
    for (const char* path : paths) {
        if (path == nullptr || *path == '\0') {
            continue;
        }
        if (const auto value = GetNamespacedValue(path); value) {
            if (const auto parsed = TryParseFloatValue(*value)) {
                return *parsed;
            }
            log.warn("Config '{}' cannot be interpreted as float", path);
        }
    }
    return defaultValue;
}

float RequiredFloat(std::string_view path) {
    if (path.empty()) {
        throw std::runtime_error("Missing required float config path");
    }
    const auto parsedPath = ParseRequiredNamespacedPath(path);
    if (const auto value = kconfig::store::Get(parsedPath.name, parsedPath.path); value) {
        if (const auto parsed = TryParseFloatValue(*value)) {
            return *parsed;
        }
        throw std::runtime_error(std::string("Invalid required float config: ") + std::string(path));
    }
    throw std::runtime_error(std::string("Missing required float config: ") + std::string(path));
}

float PositiveFiniteFloat(std::initializer_list<const char*> paths, float defaultValue) {
    const float value = Float(paths, defaultValue);
    if (!std::isfinite(value) || value <= 0.0f) {
        return defaultValue;
    }
    return value;
}

float RequiredPositiveFiniteFloat(std::string_view path) {
    const float value = RequiredFloat(path);
    if (!std::isfinite(value) || value <= 0.0f) {
        throw std::runtime_error(std::string("Invalid required float config: ")
                                 + std::string(path)
                                 + " (must be finite and > 0)");
    }
    return value;
}

std::string String(std::string_view path, const std::string& defaultValue) {
    const auto log = kconfig::GetTraceLogger();
    if (path.empty()) {
        return defaultValue;
    }
    if (const auto value = GetNamespacedValue(path); value) {
        if (value->is_string()) {
            return value->get<std::string>();
        }
        log.warn("Config '{}' cannot be interpreted as string", std::string(path));
    }
    return defaultValue;
}

std::string RequiredString(std::string_view path) {
    if (path.empty()) {
        throw std::runtime_error("Missing required string config path");
    }
    const auto parsedPath = ParseRequiredNamespacedPath(path);
    if (const auto value = kconfig::store::Get(parsedPath.name, parsedPath.path); value) {
        if (value->is_string()) {
            return value->get<std::string>();
        }
        throw std::runtime_error(std::string("Invalid required string config: ") + std::string(path));
    }
    throw std::runtime_error(std::string("Missing required string config: ") + std::string(path));
}

std::string NonEmptyString(std::string_view path, const std::string& defaultValue) {
    const std::string value = String(path, defaultValue);
    if (value.empty()) {
        return defaultValue;
    }
    return value;
}

std::string RequiredNonEmptyString(std::string_view path) {
    const std::string value = RequiredString(path);
    if (value.empty()) {
        throw std::runtime_error(std::string("Invalid required string config: ")
                                 + std::string(path)
                                 + " (must be non-empty)");
    }
    return value;
}

std::vector<float> FloatArray(std::string_view path, std::vector<float> defaultValue) {
    const auto log = kconfig::GetTraceLogger();
    const auto value = GetNamespacedValue(path);
    if (!value) {
        return defaultValue;
    }
    if (const auto parsed = TryParseFloatArrayValue(*value)) {
        return *parsed;
    }
    log.warn("Config '{}' cannot be interpreted as float array", std::string(path));
    return defaultValue;
}

std::vector<float> RequiredFloatArray(std::string_view path) {
    const auto parsedPath = ParseRequiredNamespacedPath(path);
    const auto value = kconfig::store::Get(parsedPath.name, parsedPath.path);
    if (!value) {
        throw std::runtime_error(std::string("Missing required float array config: ") + std::string(path));
    }
    if (const auto parsed = TryParseFloatArrayValue(*value)) {
        return *parsed;
    }
    throw std::runtime_error(std::string("Invalid required float array config: ") + std::string(path));
}

std::vector<std::string> StringArray(std::string_view path, std::vector<std::string> defaultValue) {
    const auto log = kconfig::GetTraceLogger();
    const auto value = GetNamespacedValue(path);
    if (!value) {
        return defaultValue;
    }
    if (const auto parsed = TryParseStringArrayValue(*value)) {
        return *parsed;
    }
    log.warn("Config '{}' cannot be interpreted as string array", std::string(path));
    return defaultValue;
}

std::vector<std::string> RequiredStringArray(std::string_view path) {
    const auto parsedPath = ParseRequiredNamespacedPath(path);
    const auto value = kconfig::store::Get(parsedPath.name, parsedPath.path);
    if (!value) {
        throw std::runtime_error(std::string("Missing required string array config: ") + std::string(path));
    }
    if (const auto parsed = TryParseStringArrayValue(*value)) {
        return *parsed;
    }
    throw std::runtime_error(std::string("Invalid required string array config: ") + std::string(path));
}

kconfig::json::Value Object(std::string_view path, kconfig::json::Value defaultValue) {
    const auto log = kconfig::GetTraceLogger();
    const auto value = GetNamespacedValue(path);
    if (!value) {
        return defaultValue;
    }
    if (!value->is_object()) {
        log.warn("Config '{}' cannot be interpreted as object", std::string(path));
        return defaultValue;
    }
    return *value;
}

kconfig::json::Value RequiredObject(std::string_view path) {
    const auto parsedPath = ParseRequiredNamespacedPath(path);
    const auto value = kconfig::store::Get(parsedPath.name, parsedPath.path);
    if (!value) {
        throw std::runtime_error(std::string("Missing required object config: ") + std::string(path));
    }
    if (!value->is_object()) {
        throw std::runtime_error(std::string("Invalid required object config: ") + std::string(path));
    }
    return *value;
}

} // namespace kconfig::store::read
