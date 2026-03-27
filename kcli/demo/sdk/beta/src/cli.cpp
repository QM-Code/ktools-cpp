#include <beta/sdk.hpp>

#include <kcli.hpp>

#include <charconv>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <system_error>

namespace {

int ParseIntOrThrow(std::string_view value) {
    int parsed = 0;
    const char* begin = value.data();
    const char* end = begin + value.size();

    const auto [ptr, ec] = std::from_chars(begin, end, parsed);
    if (ec == std::errc::result_out_of_range) {
        throw std::runtime_error("integer is out of range");
    }
    if (ec != std::errc{} || ptr != end) {
        throw std::runtime_error("expected an integer");
    }

    return parsed;
}

void PrintProcessingLine(const kcli::HandlerContext& context, std::string_view value) {
    if (context.value_tokens.empty()) {
        std::cout << "Processing " << context.option << "\n";
        return;
    }

    if (context.value_tokens.size() == 1) {
        std::cout << "Processing " << context.option
                  << " with value \"" << value << "\"\n";
        return;
    }

    std::cout << "Processing " << context.option << " with values [";
    for (std::size_t i = 0; i < context.value_tokens.size(); ++i) {
        if (i > 0) {
            std::cout << ",";
        }
        std::cout << "\"" << context.value_tokens[i] << "\"";
    }
    std::cout << "]\n";
}

void handleProfile(const kcli::HandlerContext& context, std::string_view value) {
    PrintProcessingLine(context, value);
}

void handleWorkers(const kcli::HandlerContext& context, std::string_view value) {
    if (!value.empty()) {
        (void)ParseIntOrThrow(value);
    }
    PrintProcessingLine(context, value);
}

} // namespace

namespace kcli::demo::beta {

kcli::InlineParser GetInlineParser() {
    kcli::InlineParser parser("--beta");
    parser.setHandler("-profile", handleProfile, "Select beta runtime profile.");
    parser.setHandler("-workers", handleWorkers, "Set beta worker count.");
    return parser;
}

} // namespace kcli::demo::beta
