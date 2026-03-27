#include "backend.hpp"

#include "util.hpp"

#include <algorithm>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using kcli::detail::AliasBinding;
using kcli::detail::CommandBinding;
using kcli::detail::InlineParserData;
using kcli::detail::ParseOutcome;
using kcli::detail::ParserData;

enum class InvocationKind {
    Flag,
    Value,
    Positional,
    PrintHelp,
};

struct Invocation {
    InvocationKind kind = InvocationKind::Flag;
    std::string root{};
    std::string option{};
    std::string command{};
    std::vector<std::string> value_tokens{};
    kcli::FlagHandler flag_handler{};
    kcli::ValueHandler value_handler{};
    kcli::PositionalHandler positional_handler{};
    std::vector<std::pair<std::string, std::string>> help_rows{};
};

struct CollectedValues {
    bool has_value = false;
    std::vector<std::string> parts{};
    int last_index = -1;
};

struct InlineTokenMatch {
    enum class Kind {
        None,
        BareRoot,
        DashOption,
    };

    Kind kind = Kind::None;
    const InlineParserData* parser = nullptr;
    std::string suffix{};
};

bool IsCollectableFollowOnValueToken(std::string_view value) {
    if (!value.empty() && value.front() == '-') {
        return false;
    }
    return true;
}

std::string JoinWithSpaces(const std::vector<std::string>& parts) {
    if (parts.empty()) {
        return {};
    }

    std::size_t total = 0;
    for (const std::string& part : parts) {
        total += part.size();
    }
    total += parts.size() - 1;

    std::string joined;
    joined.reserve(total);
    for (std::size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) {
            joined.push_back(' ');
        }
        joined.append(parts[i]);
    }
    return joined;
}

std::string FormatOptionErrorMessage(std::string_view option, std::string_view message) {
    if (option.empty()) {
        return std::string(message);
    }
    return "option '" + std::string(option) + "': " + std::string(message);
}

void ReportError(ParseOutcome& result,
                 std::string_view option,
                 std::string_view message) {
    if (result.ok) {
        result.ok = false;
        result.error_option = std::string(option);
        result.error_message = std::string(message);
    }
}

CollectedValues CollectValueTokens(int option_index,
                                   const std::vector<std::string>& tokens,
                                   std::vector<bool>& consumed,
                                   bool allow_option_like_first_value) {
    CollectedValues collected{};
    collected.last_index = option_index;

    const int first_value_index = option_index + 1;
    const bool has_next =
        first_value_index < static_cast<int>(tokens.size()) &&
        first_value_index >= 0 &&
        !consumed[static_cast<std::size_t>(first_value_index)];

    if (!has_next) {
        return collected;
    }

    const std::string& first = tokens[static_cast<std::size_t>(first_value_index)];
    if (!allow_option_like_first_value && !first.empty() && first.front() == '-') {
        return collected;
    }

    collected.has_value = true;
    collected.parts.push_back(first);
    consumed[static_cast<std::size_t>(first_value_index)] = true;
    collected.last_index = first_value_index;

    if (allow_option_like_first_value && !first.empty() && first.front() == '-') {
        return collected;
    }

    for (int scan = first_value_index + 1; scan < static_cast<int>(tokens.size()); ++scan) {
        if (consumed[static_cast<std::size_t>(scan)]) {
            continue;
        }

        const std::string& next = tokens[static_cast<std::size_t>(scan)];
        if (!IsCollectableFollowOnValueToken(next)) {
            break;
        }

        collected.parts.push_back(next);
        consumed[static_cast<std::size_t>(scan)] = true;
        collected.last_index = scan;
    }

    return collected;
}

void PrintHelp(const Invocation& invocation) {
    std::cout << "\nAvailable --" << invocation.root << "-* options:\n";

    std::size_t max_lhs = 0;
    for (const auto& row : invocation.help_rows) {
        max_lhs = std::max(max_lhs, row.first.size());
    }

    if (invocation.help_rows.empty()) {
        std::cout << "  (no options registered)\n";
    } else {
        for (const auto& row : invocation.help_rows) {
            std::cout << "  " << row.first;
            const std::size_t padding = max_lhs > row.first.size() ? (max_lhs - row.first.size()) : 0;
            for (std::size_t i = 0; i < padding + 2; ++i) {
                std::cout.put(' ');
            }
            std::cout << row.second << "\n";
        }
    }
    std::cout << "\n";
}

void ConsumeIndex(std::vector<bool>& consumed,
                  int index) {
    if (index < 0 || static_cast<std::size_t>(index) >= consumed.size()) {
        return;
    }

    if (!consumed[static_cast<std::size_t>(index)]) {
        consumed[static_cast<std::size_t>(index)] = true;
    }
}

const CommandBinding* FindCommand(const std::vector<std::pair<std::string, CommandBinding>>& commands,
                                  std::string_view command) {
    for (const auto& entry : commands) {
        if (entry.first == command) {
            return &entry.second;
        }
    }
    return nullptr;
}

const AliasBinding* FindAliasBinding(const ParserData& data, std::string_view token) {
    for (const AliasBinding& alias : data.aliases) {
        if (token == alias.alias) {
            return &alias;
        }
    }
    return nullptr;
}

bool HasAliasPresetTokens(const AliasBinding* alias_binding) {
    return alias_binding != nullptr && !alias_binding->preset_tokens.empty();
}

std::vector<std::string> BuildEffectiveValueTokens(const AliasBinding* alias_binding,
                                                   const std::vector<std::string>& collected_parts) {
    if (!HasAliasPresetTokens(alias_binding)) {
        return collected_parts;
    }

    std::vector<std::string> merged;
    merged.reserve(alias_binding->preset_tokens.size() + collected_parts.size());
    for (const std::string& token : alias_binding->preset_tokens) {
        merged.push_back(token);
    }
    for (const std::string& token : collected_parts) {
        merged.push_back(token);
    }
    return merged;
}

std::vector<std::pair<std::string, std::string>> BuildHelpRows(const InlineParserData& parser) {
    const std::string prefix = "--" + parser.root_name + "-";
    std::vector<std::pair<std::string, std::string>> rows;
    rows.reserve(parser.commands.size() +
                 ((parser.root_value_handler && !parser.root_value_description.empty()) ? 1u : 0u));

    if (parser.root_value_handler && !parser.root_value_description.empty()) {
        std::string lhs = "--" + parser.root_name;
        if (!parser.root_value_placeholder.empty()) {
            lhs.push_back(' ');
            lhs.append(parser.root_value_placeholder);
        }
        rows.emplace_back(std::move(lhs), parser.root_value_description);
    }

    for (const auto& [command, binding] : parser.commands) {
        std::string lhs = prefix + command;
        if (binding.expects_value) {
            if (binding.value_arity == kcli::detail::ValueArity::Optional) {
                lhs.append(" [value]");
            } else if (binding.value_arity == kcli::detail::ValueArity::Required) {
                lhs.append(" <value>");
            }
        }
        rows.emplace_back(std::move(lhs), binding.description);
    }

    return rows;
}

InlineTokenMatch MatchInlineToken(const ParserData& data, std::string_view arg) {
    for (const InlineParserData& parser : data.inline_parsers) {
        const std::string root_option = "--" + parser.root_name;
        if (arg == root_option) {
            return {.kind = InlineTokenMatch::Kind::BareRoot, .parser = &parser};
        }

        const std::string root_dash_prefix = root_option + "-";
        if (kcli::detail::StartsWith(arg, root_dash_prefix)) {
            return {
                .kind = InlineTokenMatch::Kind::DashOption,
                .parser = &parser,
                .suffix = std::string(arg.substr(root_dash_prefix.size())),
            };
        }
    }

    return {};
}

bool ScheduleInvocation(const CommandBinding& binding,
                        const AliasBinding* alias_binding,
                        std::string_view root,
                        std::string_view command,
                        std::string_view option_token,
                        int& i,
                        const std::vector<std::string>& tokens,
                        std::vector<bool>& consumed,
                        std::vector<Invocation>& invocations,
                        ParseOutcome& result) {
    ConsumeIndex(consumed, i);

    Invocation invocation{};
    invocation.root = std::string(root);
    invocation.option = std::string(option_token);
    invocation.command = std::string(command);

    if (!binding.expects_value) {
        if (HasAliasPresetTokens(alias_binding)) {
            ReportError(result,
                        alias_binding->alias,
                        "alias '" + alias_binding->alias + "' presets values for option '" +
                            std::string(option_token) + "' which does not accept values");
            return true;
        }
        invocation.kind = InvocationKind::Flag;
        invocation.flag_handler = binding.flag_handler;
        invocations.push_back(std::move(invocation));
        return true;
    }

    const CollectedValues collected = CollectValueTokens(i,
                                                         tokens,
                                                         consumed,
                                                         binding.value_arity == kcli::detail::ValueArity::Required);

    if (!collected.has_value &&
        !HasAliasPresetTokens(alias_binding) &&
        binding.value_arity == kcli::detail::ValueArity::Required) {
        ReportError(result, option_token, "option '" + std::string(option_token) + "' requires a value");
        return true;
    }

    if (collected.has_value) {
        i = collected.last_index;
    }

    invocation.kind = InvocationKind::Value;
    invocation.value_handler = binding.value_handler;
    invocation.value_tokens = BuildEffectiveValueTokens(alias_binding, collected.parts);
    invocations.push_back(std::move(invocation));
    return true;
}

void SchedulePositionals(const ParserData& data,
                        const std::vector<std::string>& tokens,
                        std::vector<bool>& consumed,
                        std::vector<Invocation>& invocations) {
    if (!data.positional_handler || tokens.size() <= 1) {
        return;
    }

    Invocation invocation{};
    invocation.kind = InvocationKind::Positional;
    invocation.positional_handler = data.positional_handler;

    for (int i = 1; i < static_cast<int>(tokens.size()); ++i) {
        if (consumed[static_cast<std::size_t>(i)]) {
            continue;
        }

        const std::string& token = tokens[static_cast<std::size_t>(i)];
        if (token.empty() || token.front() != '-') {
            consumed[static_cast<std::size_t>(i)] = true;
            invocation.value_tokens.emplace_back(token);
        }
    }

    if (!invocation.value_tokens.empty()) {
        invocations.push_back(std::move(invocation));
    }
}

std::vector<std::string> BuildParseTokens(int argc,
                                          char* const* argv) {
    std::vector<std::string> tokens(static_cast<std::size_t>(argc));
    for (int i = 0; i < argc; ++i) {
        if (argv[i] == nullptr) {
            continue;
        }
        tokens[static_cast<std::size_t>(i)] = std::string(argv[i]);
    }
    return tokens;
}

void ExecuteInvocations(const std::vector<Invocation>& invocations,
                       ParseOutcome& result) {
    for (const Invocation& invocation : invocations) {
        if (!result.ok) {
            return;
        }

        if (invocation.kind == InvocationKind::PrintHelp) {
            PrintHelp(invocation);
            continue;
        }

        kcli::HandlerContext context{};
        context.root = invocation.root;
        context.option = invocation.option;
        context.command = invocation.command;
        context.value_tokens.reserve(invocation.value_tokens.size());
        for (const std::string& token : invocation.value_tokens) {
            context.value_tokens.push_back(token);
        }

        try {
            switch (invocation.kind) {
            case InvocationKind::Flag:
                invocation.flag_handler(context);
                break;
            case InvocationKind::Value:
                invocation.value_handler(context, JoinWithSpaces(invocation.value_tokens));
                break;
            case InvocationKind::Positional:
                invocation.positional_handler(context);
                break;
            case InvocationKind::PrintHelp:
                break;
            }
        } catch (const std::exception& ex) {
            ReportError(result, invocation.option, FormatOptionErrorMessage(invocation.option, ex.what()));
        } catch (...) {
            ReportError(result,
                        invocation.option,
                        FormatOptionErrorMessage(invocation.option,
                                                 "unknown exception while handling option"));
        }
    }
}

}  // namespace

namespace kcli::detail {

void Parse(ParserData& data, int argc, char* const* argv) {
    ParseOutcome result{};
    if (argc > 0 && argv == nullptr) {
        result = MakeError("", "kcli received invalid argv (argc > 0 but argv is null)");
        ThrowCliError(result);
    }

    if (argc <= 0 || argv == nullptr) {
        return;
    }

    std::vector<bool> consumed(static_cast<std::size_t>(argc), false);
    std::vector<Invocation> invocations;
    std::vector<std::string> tokens = BuildParseTokens(argc, argv);

    for (int i = 1; i < argc; ++i) {
        if (consumed[static_cast<std::size_t>(i)]) {
            continue;
        }

        std::string& arg = tokens[static_cast<std::size_t>(i)];
        if (arg.empty()) {
            continue;
        }

        const AliasBinding* alias_binding = nullptr;
        if (arg.front() == '-' && !StartsWith(arg, "--")) {
            alias_binding = FindAliasBinding(data, arg);
            if (alias_binding != nullptr) {
                arg = alias_binding->target_token;
            }
        }

        if (arg.front() != '-') {
            continue;
        }

        if (arg == "--") {
            continue;
        }

        if (StartsWith(arg, "--")) {
            const InlineTokenMatch inline_match = MatchInlineToken(data, arg);
            switch (inline_match.kind) {
            case InlineTokenMatch::Kind::BareRoot: {
                ConsumeIndex(consumed, i);
                const CollectedValues collected =
                    CollectValueTokens(i, tokens, consumed, false);

                if (!collected.has_value && !HasAliasPresetTokens(alias_binding)) {
                    Invocation help{};
                    help.kind = InvocationKind::PrintHelp;
                    help.root = inline_match.parser->root_name;
                    help.help_rows = BuildHelpRows(*inline_match.parser);
                    invocations.push_back(std::move(help));
                    break;
                }

                if (!inline_match.parser->root_value_handler) {
                    ReportError(result,
                                arg,
                                "unknown value for option '" + arg + "'");
                    break;
                }

                Invocation invocation{};
                invocation.kind = InvocationKind::Value;
                invocation.root = inline_match.parser->root_name;
                invocation.option = arg;
                invocation.value_handler = inline_match.parser->root_value_handler;
                invocation.value_tokens = BuildEffectiveValueTokens(alias_binding, collected.parts);
                invocations.push_back(std::move(invocation));
                if (collected.has_value) {
                    i = collected.last_index;
                }
                break;
            }
            case InlineTokenMatch::Kind::DashOption: {
                const CommandBinding* binding =
                    FindCommand(inline_match.parser->commands, inline_match.suffix);
                if (inline_match.suffix.empty() || binding == nullptr) {
                    break;
                }

                (void)ScheduleInvocation(*binding,
                                         alias_binding,
                                         inline_match.parser->root_name,
                                         inline_match.suffix,
                                         arg,
                                         i,
                                         tokens,
                                         consumed,
                                         invocations,
                                         result);
                break;
            }
            case InlineTokenMatch::Kind::None: {
                const std::string command = arg.substr(2);
                const CommandBinding* binding = FindCommand(data.commands, command);
                if (binding != nullptr) {
                    (void)ScheduleInvocation(*binding,
                                             alias_binding,
                                             "",
                                             command,
                                             arg,
                                             i,
                                             tokens,
                                             consumed,
                                             invocations,
                                             result);
                }
                break;
            }
            }
        }

        if (!result.ok) {
            break;
        }
    }

    if (result.ok) {
        SchedulePositionals(data, tokens, consumed, invocations);
    }

    if (result.ok) {
        for (int i = 1; i < argc; ++i) {
            if (consumed[static_cast<std::size_t>(i)]) {
                continue;
            }

            const std::string& token = tokens[static_cast<std::size_t>(i)];
            if (token.empty()) {
                continue;
            }
            if (token.front() == '-') {
                ReportError(result, token, "unknown option " + token);
                break;
            }
        }
    }

    if (result.ok) {
        ExecuteInvocations(invocations, result);
    }

    if (!result.ok) {
        ThrowCliError(result);
    }
}

}  // namespace kcli::detail
