#include <kcli.hpp>

#include "kcli/backend.hpp"

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <utility>

#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

namespace kcli {

namespace {

bool StderrSupportsColor() {
#if defined(_WIN32)
    return _isatty(_fileno(stderr)) != 0;
#else
    return isatty(fileno(stderr)) != 0;
#endif
}

[[noreturn]] void ReportCliErrorAndExit(const char* message) {
    if (StderrSupportsColor()) {
        std::fprintf(stderr, "[\x1b[31merror\x1b[0m] [\x1b[94mcli\x1b[0m] %s\n", message);
    } else {
        std::fprintf(stderr, "[error] [cli] %s\n", message);
    }
    std::fflush(stderr);
    std::exit(2);
}

}  // namespace

CliError::CliError(std::string option, std::string message)
    : std::runtime_error(message.empty() ? "kcli parse failed" : message),
      option_(std::move(option)) {
}

CliError::~CliError() = default;

std::string_view CliError::option() const noexcept {
    return option_;
}

InlineParser::InlineParser(std::string_view root)
    : data_(std::make_unique<detail::InlineParserData>()) {
    detail::SetInlineRoot(*data_, root);
}

InlineParser::InlineParser(const InlineParser& other)
    : data_(std::make_unique<detail::InlineParserData>(
          detail::CloneInlineParserData(*other.data_))) {
}

InlineParser& InlineParser::operator=(const InlineParser& other) {
    if (this == &other) {
        return *this;
    }

    *data_ = detail::CloneInlineParserData(*other.data_);
    return *this;
}

InlineParser::InlineParser(InlineParser&& other) noexcept = default;

InlineParser& InlineParser::operator=(InlineParser&& other) noexcept = default;

InlineParser::~InlineParser() = default;

void InlineParser::setRoot(std::string_view root) {
    detail::SetInlineRoot(*data_, root);
}

void InlineParser::setRootValueHandler(ValueHandler handler) {
    detail::SetRootValueHandler(*data_, std::move(handler));
}

void InlineParser::setRootValueHandler(ValueHandler handler,
                                       std::string_view value_placeholder,
                                       std::string_view description) {
    detail::SetRootValueHandler(
        *data_, std::move(handler), value_placeholder, description);
}

void InlineParser::setHandler(std::string_view option,
                              FlagHandler handler,
                              std::string_view description) {
    detail::SetInlineHandler(*data_, option, std::move(handler), description);
}

void InlineParser::setHandler(std::string_view option,
                              ValueHandler handler,
                              std::string_view description) {
    detail::SetInlineHandler(*data_, option, std::move(handler), description);
}

void InlineParser::setOptionalValueHandler(std::string_view option,
                                           ValueHandler handler,
                                           std::string_view description) {
    detail::SetInlineOptionalValueHandler(*data_, option, std::move(handler), description);
}

Parser::Parser()
    : data_(std::make_unique<detail::ParserData>()) {
}

Parser::Parser(Parser&& other) noexcept = default;

Parser& Parser::operator=(Parser&& other) noexcept = default;

Parser::~Parser() = default;

void Parser::addAlias(std::string_view alias, std::string_view target) {
    detail::SetAlias(*data_, alias, target);
}

void Parser::addAlias(std::string_view alias,
                      std::string_view target,
                      std::initializer_list<std::string_view> preset_tokens) {
    detail::SetAlias(*data_, alias, target, preset_tokens);
}

void Parser::setHandler(std::string_view option,
                        FlagHandler handler,
                        std::string_view description) {
    detail::SetPrimaryHandler(*data_, option, std::move(handler), description);
}

void Parser::setHandler(std::string_view option,
                        ValueHandler handler,
                        std::string_view description) {
    detail::SetPrimaryHandler(*data_, option, std::move(handler), description);
}

void Parser::setOptionalValueHandler(std::string_view option,
                                     ValueHandler handler,
                                     std::string_view description) {
    detail::SetPrimaryOptionalValueHandler(*data_, option, std::move(handler), description);
}

void Parser::setPositionalHandler(PositionalHandler handler) {
    detail::SetPositionalHandler(*data_, std::move(handler));
}

void Parser::addInlineParser(InlineParser parser) {
    detail::AddInlineParser(*data_, detail::CloneInlineParserData(*parser.data_));
}

void Parser::parseOrExit(int argc, char* const* argv) {
    try {
        parseOrThrow(argc, argv);
    } catch (const CliError& ex) {
        ReportCliErrorAndExit(ex.what());
    }
}

void Parser::parseOrThrow(int argc, char* const* argv) {
    detail::Parse(*data_, argc, argv);
}

}  // namespace kcli
