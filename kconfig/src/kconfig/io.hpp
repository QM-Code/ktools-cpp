#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include <kconfig/json.hpp>

namespace kconfig::io {

std::filesystem::path Canonicalize(const std::filesystem::path& path);

enum class JsonReadError {
    None,
    NotFound,
    OpenFailed,
    ParseFailed
};

struct JsonReadResult {
    std::optional<kconfig::json::Value> json{};
    JsonReadError error = JsonReadError::None;
    std::string message{};
};

JsonReadResult ReadJsonFile(const std::filesystem::path& path);
bool EnsureJsonObjectFile(const std::filesystem::path& path, std::string* error = nullptr);
bool WriteJsonFile(const std::filesystem::path& path,
                   const kconfig::json::Value& json,
                   std::string* error = nullptr);

void MergeJsonObjects(kconfig::json::Value& destination,
                      const kconfig::json::Value& source);

} // namespace kconfig::io
