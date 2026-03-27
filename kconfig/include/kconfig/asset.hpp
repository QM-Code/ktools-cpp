#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <kconfig/json.hpp>

namespace kconfig::asset {

std::optional<std::filesystem::path> ResolvePath(std::string_view name,
                                                 std::string_view jsonPath);

std::filesystem::path RequiredPath(std::string_view name,
                                   std::string_view jsonPath);

bool Exists(std::string_view name,
            std::string_view jsonPath);

std::vector<std::uint8_t> LoadBytes(std::string_view name,
                                    std::string_view jsonPath,
                                    std::vector<std::uint8_t> defaultValue = {});

std::vector<std::uint8_t> LoadRequiredBytes(std::string_view name,
                                            std::string_view jsonPath);

std::string LoadText(std::string_view name,
                     std::string_view jsonPath,
                     std::string defaultValue = {});

std::string LoadRequiredText(std::string_view name,
                             std::string_view jsonPath);

kconfig::json::Value LoadJson(std::string_view name,
                              std::string_view jsonPath,
                              kconfig::json::Value defaultValue = kconfig::json::Object());

kconfig::json::Value LoadRequiredJson(std::string_view name,
                                      std::string_view jsonPath);

} // namespace kconfig::asset
