#pragma once

#include <kcli.hpp>

#include <initializer_list>
#include <string>
#include <vector>

namespace kcli::detail {

enum class ValueArity {
    Required,
    Optional,
};

struct CommandBinding {
    bool expects_value = false;
    FlagHandler flag_handler{};
    ValueHandler value_handler{};
    ValueArity value_arity = ValueArity::Required;
    std::string description{};
};

struct AliasBinding {
    std::string alias{};
    std::string target_token{};
    std::vector<std::string> preset_tokens{};
};

struct InlineParserData {
    std::string root_name{};
    ValueHandler root_value_handler{};
    std::string root_value_placeholder{};
    std::string root_value_description{};
    std::vector<std::pair<std::string, CommandBinding>> commands{};
};

struct ParserData {
    PositionalHandler positional_handler{};
    std::vector<AliasBinding> aliases{};
    std::vector<std::pair<std::string, CommandBinding>> commands{};
    std::vector<InlineParserData> inline_parsers{};
};

InlineParserData CloneInlineParserData(const InlineParserData& data);

void SetInlineRoot(InlineParserData& data, std::string_view root);
void SetRootValueHandler(InlineParserData& data, ValueHandler handler);
void SetRootValueHandler(InlineParserData& data,
                         ValueHandler handler,
                         std::string_view value_placeholder,
                         std::string_view description);
void SetInlineHandler(InlineParserData& data,
                      std::string_view option,
                      FlagHandler handler,
                      std::string_view description);
void SetInlineHandler(InlineParserData& data,
                      std::string_view option,
                      ValueHandler handler,
                      std::string_view description);
void SetInlineOptionalValueHandler(InlineParserData& data,
                                   std::string_view option,
                                   ValueHandler handler,
                                   std::string_view description);

void SetAlias(ParserData& data, std::string_view alias, std::string_view target);
void SetAlias(ParserData& data,
              std::string_view alias,
              std::string_view target,
              std::initializer_list<std::string_view> preset_tokens);
void SetPrimaryHandler(ParserData& data,
                       std::string_view option,
                       FlagHandler handler,
                       std::string_view description);
void SetPrimaryHandler(ParserData& data,
                       std::string_view option,
                       ValueHandler handler,
                       std::string_view description);
void SetPrimaryOptionalValueHandler(ParserData& data,
                                    std::string_view option,
                                    ValueHandler handler,
                                    std::string_view description);
void SetPositionalHandler(ParserData& data, PositionalHandler handler);
void AddInlineParser(ParserData& data, InlineParserData parser);
void Parse(ParserData& data, int argc, char* const* argv);

}  // namespace kcli::detail
