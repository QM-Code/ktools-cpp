#include <alpha/sdk.hpp>

#include <kcli.hpp>

#include <cstddef>
#include <iostream>
#include <stdexcept>

namespace {

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

void handleMessage(const kcli::HandlerContext& context, std::string_view value) {
    PrintProcessingLine(context, value);
}

void handleEnable(const kcli::HandlerContext& context, std::string_view value) {
    PrintProcessingLine(context, value);
}

} // namespace

namespace kcli::demo::alpha {

kcli::InlineParser GetInlineParser() {
    kcli::InlineParser parser("--alpha");
    parser.setHandler("-message", handleMessage, "Set alpha message label.");
    parser.setOptionalValueHandler("-enable", handleEnable, "Enable alpha processing.");
    return parser;
}

} // namespace kcli::demo::alpha
