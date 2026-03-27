#include "backend.hpp"

#include "util.hpp"

#include <stdexcept>
#include <utility>

namespace {

using kcli::detail::CommandBinding;
using kcli::detail::InlineParserData;
using kcli::detail::ParserData;

CommandBinding MakeFlagBinding(kcli::FlagHandler handler, std::string_view description) {
    if (!handler) {
        throw std::invalid_argument("kcli flag handler must not be empty");
    }

    CommandBinding binding{};
    binding.expects_value = false;
    binding.flag_handler = std::move(handler);
    binding.description = kcli::detail::NormalizeDescriptionOrThrow(description);
    return binding;
}

CommandBinding MakeValueBinding(kcli::ValueHandler handler,
                                std::string_view description,
                                kcli::detail::ValueArity arity) {
    if (!handler) {
        throw std::invalid_argument("kcli value handler must not be empty");
    }

    CommandBinding binding{};
    binding.expects_value = true;
    binding.value_handler = std::move(handler);
    binding.value_arity = arity;
    binding.description = kcli::detail::NormalizeDescriptionOrThrow(description);
    return binding;
}

void UpsertCommand(std::vector<std::pair<std::string, CommandBinding>>& commands,
                   std::string command,
                   CommandBinding binding) {
    for (auto& entry : commands) {
        if (entry.first != command) {
            continue;
        }

        entry.second = std::move(binding);
        return;
    }

    commands.emplace_back(std::move(command), std::move(binding));
}

}  // namespace

namespace kcli::detail {

InlineParserData CloneInlineParserData(const InlineParserData& data) {
    return data;
}

void SetInlineRoot(InlineParserData& data, std::string_view root) {
    data.root_name = NormalizeInlineRootOptionOrThrow(root);
}

void SetRootValueHandler(InlineParserData& data, ValueHandler handler) {
    if (!handler) {
        throw std::invalid_argument("kcli root value handler must not be empty");
    }
    data.root_value_handler = std::move(handler);
    data.root_value_placeholder.clear();
    data.root_value_description.clear();
}

void SetRootValueHandler(InlineParserData& data,
                         ValueHandler handler,
                         std::string_view value_placeholder,
                         std::string_view description) {
    if (!handler) {
        throw std::invalid_argument("kcli root value handler must not be empty");
    }
    data.root_value_handler = std::move(handler);
    data.root_value_placeholder = NormalizeHelpPlaceholderOrThrow(value_placeholder);
    data.root_value_description = NormalizeDescriptionOrThrow(description);
}

void SetInlineHandler(InlineParserData& data,
                      std::string_view option,
                      FlagHandler handler,
                      std::string_view description) {
    const std::string command = NormalizeInlineHandlerOptionOrThrow(option, data.root_name);
    UpsertCommand(data.commands, command, MakeFlagBinding(std::move(handler), description));
}

void SetInlineHandler(InlineParserData& data,
                      std::string_view option,
                      ValueHandler handler,
                      std::string_view description) {
    const std::string command = NormalizeInlineHandlerOptionOrThrow(option, data.root_name);
    UpsertCommand(
        data.commands, command, MakeValueBinding(std::move(handler), description, ValueArity::Required));
}

void SetInlineOptionalValueHandler(InlineParserData& data,
                                   std::string_view option,
                                   ValueHandler handler,
                                   std::string_view description) {
    const std::string command = NormalizeInlineHandlerOptionOrThrow(option, data.root_name);
    UpsertCommand(
        data.commands, command, MakeValueBinding(std::move(handler), description, ValueArity::Optional));
}

void SetAlias(ParserData& data, std::string_view alias, std::string_view target) {
    SetAlias(data, alias, target, {});
}

void SetAlias(ParserData& data,
              std::string_view alias,
              std::string_view target,
              std::initializer_list<std::string_view> preset_tokens) {
    const std::string normalized_alias = NormalizeAliasOrThrow(alias);
    const std::string normalized_target = NormalizeAliasTargetOptionOrThrow(target);

    AliasBinding normalized_binding{};
    normalized_binding.alias = normalized_alias;
    normalized_binding.target_token = normalized_target;
    normalized_binding.preset_tokens.reserve(preset_tokens.size());
    for (std::string_view token : preset_tokens) {
        normalized_binding.preset_tokens.emplace_back(token);
    }

    for (AliasBinding& binding : data.aliases) {
        if (binding.alias != normalized_alias) {
            continue;
        }

        binding = std::move(normalized_binding);
        return;
    }

    data.aliases.push_back(std::move(normalized_binding));
}

void SetPrimaryHandler(ParserData& data,
                       std::string_view option,
                       FlagHandler handler,
                       std::string_view description) {
    const std::string command = NormalizePrimaryHandlerOptionOrThrow(option);
    UpsertCommand(data.commands, command, MakeFlagBinding(std::move(handler), description));
}

void SetPrimaryHandler(ParserData& data,
                       std::string_view option,
                       ValueHandler handler,
                       std::string_view description) {
    const std::string command = NormalizePrimaryHandlerOptionOrThrow(option);
    UpsertCommand(
        data.commands, command, MakeValueBinding(std::move(handler), description, ValueArity::Required));
}

void SetPrimaryOptionalValueHandler(ParserData& data,
                                    std::string_view option,
                                    ValueHandler handler,
                                    std::string_view description) {
    const std::string command = NormalizePrimaryHandlerOptionOrThrow(option);
    UpsertCommand(
        data.commands, command, MakeValueBinding(std::move(handler), description, ValueArity::Optional));
}

void SetPositionalHandler(ParserData& data, PositionalHandler handler) {
    if (!handler) {
        throw std::invalid_argument("kcli positional handler must not be empty");
    }
    data.positional_handler = std::move(handler);
}

void AddInlineParser(ParserData& data, InlineParserData parser) {
    for (const InlineParserData& existing : data.inline_parsers) {
        if (existing.root_name != parser.root_name) {
            continue;
        }

        throw std::invalid_argument("kcli inline parser root '--" + parser.root_name +
                                    "' is already registered");
    }

    data.inline_parsers.push_back(std::move(parser));
}

}  // namespace kcli::detail
