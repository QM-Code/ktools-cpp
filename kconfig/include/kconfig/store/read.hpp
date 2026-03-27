#pragma once

#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

#include <kconfig/json.hpp>

namespace kconfig::store::read {

bool Bool(std::initializer_list<const char*> paths, bool defaultValue);
bool RequiredBool(std::string_view path);

uint16_t Uint16(std::initializer_list<const char*> paths, uint16_t defaultValue);
uint16_t RequiredUint16(std::string_view path);

uint16_t PositiveUint16(std::initializer_list<const char*> paths, uint16_t defaultValue);
uint16_t RequiredPositiveUint16(std::string_view path);

uint32_t Uint32(std::initializer_list<const char*> paths, uint32_t defaultValue);
uint32_t RequiredUint32(std::string_view path);

float Float(std::initializer_list<const char*> paths, float defaultValue);
float RequiredFloat(std::string_view path);

float PositiveFiniteFloat(std::initializer_list<const char*> paths, float defaultValue);
float RequiredPositiveFiniteFloat(std::string_view path);

std::string String(std::string_view path, const std::string& defaultValue);
std::string RequiredString(std::string_view path);

std::string NonEmptyString(std::string_view path, const std::string& defaultValue);
std::string RequiredNonEmptyString(std::string_view path);

std::vector<float> FloatArray(std::string_view path, std::vector<float> defaultValue = {});
std::vector<float> RequiredFloatArray(std::string_view path);

std::vector<std::string> StringArray(std::string_view path, std::vector<std::string> defaultValue = {});
std::vector<std::string> RequiredStringArray(std::string_view path);

kconfig::json::Value Object(std::string_view path,
                            kconfig::json::Value defaultValue = kconfig::json::Object());
kconfig::json::Value RequiredObject(std::string_view path);

} // namespace kconfig::store::read
