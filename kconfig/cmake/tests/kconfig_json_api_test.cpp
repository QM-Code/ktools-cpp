#include <kconfig/json.hpp>

#include <sstream>
#include <stdexcept>
#include <string>

namespace {

[[noreturn]] void Fail(const std::string& message) {
    throw std::runtime_error(message);
}

void ExpectTrue(const bool condition, const std::string& message) {
    if (!condition) {
        Fail(message);
    }
}

void ExpectEqual(const std::string& actual, const std::string& expected) {
    if (actual != expected) {
        Fail("expected '" + expected + "', got '" + actual + "'");
    }
}

void ExpectEqual(const long long actual, const long long expected, const std::string& label) {
    if (actual != expected) {
        Fail(label + ": expected " + std::to_string(expected) + ", got " + std::to_string(actual));
    }
}

void ExpectEqual(const unsigned long long actual, const unsigned long long expected, const std::string& label) {
    if (actual != expected) {
        Fail(label + ": expected " + std::to_string(expected) + ", got " + std::to_string(actual));
    }
}

void ExpectEqual(const double actual, const double expected, const std::string& label) {
    if (actual != expected) {
        Fail(label + ": expected " + std::to_string(expected) + ", got " + std::to_string(actual));
    }
}

void ExpectThrowsParse(std::string_view text) {
    bool threw = false;
    try {
        (void)kconfig::json::Parse(text);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    ExpectTrue(threw, "expected parse failure for '" + std::string(text) + "'");
}

void VerifyScalarParsing() {
    const auto unsignedValue = kconfig::json::Parse("42");
    ExpectTrue(unsignedValue.is_number_unsigned(), "42 should parse as unsigned");
    ExpectEqual(unsignedValue.get<unsigned long long>(), 42ULL, "unsigned parse");

    const auto signedValue = kconfig::json::Parse("-17");
    ExpectTrue(signedValue.is_number_integer(), "-17 should parse as integer");
    ExpectEqual(signedValue.get<long long>(), -17LL, "signed parse");

    const auto floatValue = kconfig::json::Parse("7.75");
    ExpectTrue(floatValue.is_number_float(), "7.75 should parse as float");
    ExpectEqual(floatValue.get<double>(), 7.75, "float parse");

    const auto boolValue = kconfig::json::Parse("true");
    ExpectTrue(boolValue.is_boolean(), "true should parse as boolean");
    ExpectTrue(boolValue.get<bool>(), "true should parse to bool true");

    const auto stringValue = kconfig::json::Parse("\"line\\ntext\"");
    ExpectTrue(stringValue.is_string(), "string literal should parse as string");
    ExpectEqual(stringValue.get<std::string>(), "line\ntext");

    const auto unicodeValue = kconfig::json::Parse("\"\\u20AC\"");
    ExpectEqual(unicodeValue.get<std::string>(), "\xE2\x82\xAC");
}

void VerifyDumpFormatting() {
    kconfig::json::Value value = kconfig::json::Object();
    value["name"] = "demo";
    value["flag"] = true;
    value["value"] = 2.0;
    value["items"] = kconfig::json::Array();
    value["items"].push_back(1);
    value["items"].push_back(2.0);
    value["items"].push_back("x");

    ExpectEqual(value.dump(), R"({"flag":true,"items":[1,2.0,"x"],"name":"demo","value":2.0})");

    const std::string pretty = value.dump(2);
    ExpectEqual(pretty,
                "{\n"
                "  \"flag\": true,\n"
                "  \"items\": [\n"
                "    1,\n"
                "    2.0,\n"
                "    \"x\"\n"
                "  ],\n"
                "  \"name\": \"demo\",\n"
                "  \"value\": 2.0\n"
                "}");

    const auto reparsed = kconfig::json::Parse(pretty);
    ExpectTrue(reparsed["items"][1].is_number_float(), "integral-valued float should stay float after roundtrip");
    ExpectEqual(reparsed["name"].get<std::string>(), "demo");
}

void VerifyStreamRoundTrip() {
    std::stringstream stream;
    stream << kconfig::json::Parse(R"({"float":9.0,"name":"stream"})");

    kconfig::json::Value reparsed;
    stream >> reparsed;
    ExpectTrue(reparsed["float"].is_number_float(), "stream roundtrip should preserve float typing");
    ExpectEqual(reparsed["float"].get<double>(), 9.0, "stream float value");
    ExpectEqual(reparsed["name"].get<std::string>(), "stream");
}

void VerifyParseFailures() {
    ExpectThrowsParse("");
    ExpectThrowsParse("{");
    ExpectThrowsParse("01");
    ExpectThrowsParse("{\"a\":]");
    ExpectThrowsParse("\"\\uD800\"");
}

} // namespace

int main() {
    VerifyScalarParsing();
    VerifyDumpFormatting();
    VerifyStreamRoundTrip();
    VerifyParseFailures();
    return 0;
}
