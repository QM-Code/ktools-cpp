#pragma once

#include <functional>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace kcli {

// Metadata describing one scheduled handler invocation.
struct HandlerContext {

    // Inline root name without leading dashes, for example "build".
    // Empty for top-level handlers and positional dispatch.
    std::string_view root{};

    // Effective option token after alias expansion, for example "--verbose".
    // Empty for positional dispatch.
    std::string_view option{};

    // Normalized command name without leading dashes.
    // Empty for positional dispatch and inline root value handlers.
    std::string_view command{};

    // Effective value tokens for this handler after alias expansion.
    // Tokens originating from argv are preserved verbatim; alias preset
    // tokens appear ahead of user-supplied tokens.
    // For optional-value handlers, an omitted value leaves this empty;
    // an explicit empty-string argument contributes one empty token.
    std::vector<std::string_view> value_tokens{};
};

using FlagHandler = std::function<void(const HandlerContext&)>;
using ValueHandler = std::function<void(const HandlerContext&, std::string_view)>;
// Positional handlers receive remaining positional tokens in
// HandlerContext::value_tokens.
using PositionalHandler = std::function<void(const HandlerContext&)>;

// Error surfaced by parseOrThrow() for invalid CLI input or handler failures.
// option() is empty for positional-handler failures and parser-global errors.
class CliError : public std::runtime_error {
public:
    CliError(std::string option, std::string message);
    ~CliError() override;

    std::string_view option() const noexcept;

private:
    std::string option_;
};

namespace detail {
struct InlineParserData;
struct ParserData;
}  // namespace detail

class InlineParser {
public:

    // Registers one inline namespace such as "--build" or "--alpha".
    // The root may be provided as "build" or "--build".
    explicit InlineParser(std::string_view root);

    InlineParser(const InlineParser& other);
    InlineParser& operator=(const InlineParser& other);
    InlineParser(InlineParser&& other) noexcept;
    InlineParser& operator=(InlineParser&& other) noexcept;
    ~InlineParser();

    // Replaces the inline root. The root may be provided as "build" or "--build".
    void setRoot(std::string_view root);

    // Handles the bare root form, for example "--build release".
    // When no value is provided, the parser prints inline help instead.
    void setRootValueHandler(ValueHandler handler);

    // Same as above, and also adds a help row such as "--build <selector>".
    void setRootValueHandler(ValueHandler handler,
                             std::string_view value_placeholder,
                             std::string_view description);

    // Registers an inline flag handler. The option may be provided as "-name"
    // or as the fully-qualified "--<root>-name".
    void setHandler(std::string_view option,
                    FlagHandler handler,
                    std::string_view description);

    // Registers an inline value handler. The option may be provided as "-name"
    // or as the fully-qualified "--<root>-name". A value is required.
    void setHandler(std::string_view option,
                    ValueHandler handler,
                    std::string_view description);

    // Registers an inline value handler whose value is optional. The option may
    // be provided as "-name" or as the fully-qualified "--<root>-name".
    void setOptionalValueHandler(std::string_view option,
                                 ValueHandler handler,
                                 std::string_view description);

private:
    std::unique_ptr<detail::InlineParserData> data_;

    friend class Parser;
};

class Parser {
public:
    Parser();

    Parser(const Parser&) = delete;
    Parser& operator=(const Parser&) = delete;
    Parser(Parser&& other) noexcept;
    Parser& operator=(Parser&& other) noexcept;
    ~Parser();

    // Maps a single-dash alias such as "-v" to a long-form option such as
    // "--verbose". Preset tokens, when provided, are prepended to the
    // handler's effective value_tokens.
    void addAlias(std::string_view alias, std::string_view target);

    void addAlias(std::string_view alias,
                  std::string_view target,
                  std::initializer_list<std::string_view> preset_tokens);

    // Registers a top-level flag handler. The option may be provided as
    // "name" or "--name".
    void setHandler(std::string_view option,
                    FlagHandler handler,
                    std::string_view description);

    // Registers a top-level value handler. The option may be provided as
    // "name" or "--name". A value is required.
    void setHandler(std::string_view option,
                    ValueHandler handler,
                    std::string_view description);

    // Registers a top-level value handler whose value is optional. The option
    // may be provided as "name" or "--name".
    void setOptionalValueHandler(std::string_view option,
                                 ValueHandler handler,
                                 std::string_view description);

    // Receives remaining non-option tokens after option parsing succeeds.
    void setPositionalHandler(PositionalHandler handler);

    // Adds an inline parser. Duplicate roots are rejected.
    void addInlineParser(InlineParser parser);

    // Parses argv without modifying the caller's argument vector.
    // On failure, prints "[error] [cli] ..." to stderr and exits with code 2.
    void parseOrExit(int argc, char* const* argv);

    // Parses argv without modifying the caller's argument vector.
    // Throws CliError on invalid CLI input or when a handler throws.
    // No registered handlers run until the full command line validates.
    void parseOrThrow(int argc, char* const* argv);

private:
    std::unique_ptr<detail::ParserData> data_;
};

}  // namespace kcli
