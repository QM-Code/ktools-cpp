#include <alpha/sdk.hpp>
#include <beta/sdk.hpp>
#include <gamma/sdk.hpp>
#include <kcli.hpp>

#include <iostream>
#include <string_view>

namespace {

void handleBuildProfile(const kcli::HandlerContext&, std::string_view) {
}

void handleBuildClean(const kcli::HandlerContext&) {
}

void handleVerbose(const kcli::HandlerContext&) {
}

void handleOutput(const kcli::HandlerContext&, std::string_view) {
}

void handleArgs(const kcli::HandlerContext&) {
}

} // namespace

int main(int argc, char** argv) {
    kcli::Parser parser;
    kcli::InlineParser alphaParser = kcli::demo::alpha::GetInlineParser();
    kcli::InlineParser betaParser = kcli::demo::beta::GetInlineParser();
    kcli::InlineParser gammaParser = kcli::demo::gamma::GetInlineParser();
    kcli::InlineParser inlineBuildParser("--build");

    gammaParser.setRoot("--newgamma");

    parser.addInlineParser(alphaParser);
    parser.addInlineParser(betaParser);
    parser.addInlineParser(gammaParser);

    parser.addAlias("-v", "--verbose");
    parser.addAlias("-out", "--output");
    parser.addAlias("-a", "--alpha-enable");
    parser.addAlias("-b", "--build-profile");

    inlineBuildParser.setHandler("-profile", handleBuildProfile, "Set build profile.");
    inlineBuildParser.setHandler("-clean", handleBuildClean, "Enable clean build.");

    parser.addInlineParser(inlineBuildParser);

    parser.setHandler("--verbose", handleVerbose, "Enable verbose app logging.");
    parser.setHandler("--output", handleOutput, "Set app output target.");

    parser.setPositionalHandler(handleArgs);

    parser.parseOrExit(argc, argv);

    std::cout << "\nUsage:\n";
    std::cout << "  kcli_demo_omega --<root>\n\n";
    std::cout << "Enabled --<root> prefixes:\n";
    std::cout << "  --alpha\n";
    std::cout << "  --beta\n";
    std::cout << "  --newgamma (gamma override)\n\n";
    return 0;
}
