#include <alpha/sdk.hpp>
#include <ki18n.hpp>
#include <kconfig.hpp>
#include <kconfig/store.hpp>
#include <kcli.hpp>
#include <ktrace.hpp>

#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#define KTRACE(channel, ...) tracer.trace(channel, __VA_ARGS__)

int main(int argc, char** argv) {
    ktrace::Logger logger;

    ktrace::TraceLogger tracer("ki18n_demo_core");
    tracer.addChannel("store");
    tracer.addChannel("store.requests");

    logger.addTraceLogger(tracer);
    logger.addTraceLogger(kconfig::GetTraceLogger());
    logger.addTraceLogger(ki18n::GetTraceLogger());

    kcli::Parser parser;
    parser.addInlineParser(logger.makeInlineParser(tracer));
    parser.addInlineParser(kconfig::cli::GetInlineParser());

    parser.parseOrExit(argc, argv);

    const std::filesystem::path repoRoot = std::filesystem::current_path();
    const std::filesystem::path runtimeRoot = repoRoot / "demo" / "exe" / "core" / "runtime";
    const std::filesystem::path defaultsPath = runtimeRoot / "defaults.json";
    const std::filesystem::path userPath = runtimeRoot / "user.json";
    const std::filesystem::path sessionPath = runtimeRoot / "session.json";
    const std::filesystem::path i18nPath = runtimeRoot / "i18n.json";

    KTRACE("store.requests",
           "core demo preparing config load defaults='{}' user='{}' session='{}' i18n='{}'",
           defaultsPath.string(),
           userPath.string(),
           sessionPath.string(),
           i18nPath.string());

    std::string ioError;

    if (!kconfig::store::fs::LoadReadOnly("defaults", defaultsPath, &ioError)) {
        std::cerr << "Failed to load defaults config: " << ioError << "\n";
        return 1;
    }

    ioError.clear();
    if (!kconfig::store::fs::LoadMutable("user", userPath, &ioError)) {
        std::cerr << "Failed to load user config: " << ioError << "\n";
        return 1;
    }

    ioError.clear();
    if (!kconfig::store::fs::LoadReadOnly("session", sessionPath, &ioError)) {
        std::cerr << "Failed to load session config: " << ioError << "\n";
        return 1;
    }

    ioError.clear();
    if (!kconfig::store::fs::LoadReadOnly("i18n", i18nPath, &ioError)) {
        std::cerr << "Failed to load i18n config: " << ioError << "\n";
        return 1;
    }

    std::vector<std::string> mergeSources = {"defaults", "user", "session"};
    if (kconfig::store::Has("config")) {
        KTRACE("store.requests", "core demo detected CLI namespace 'config' -> append to merge sources");
        mergeSources.push_back("config");
    }
    if (!kconfig::store::Merge("merged", mergeSources)) {
        std::cerr << "Failed to merge config namespaces: defaults + user + session -> merged\n";
        return 1;
    }

    try {
        const bool enabled = kconfig::store::read::RequiredBool("merged.feature.enabled");
        const uint16_t port = kconfig::store::read::RequiredUint16("merged.network.port");
        const std::string language = kconfig::store::read::RequiredNonEmptyString("merged.client.Language");
        const std::string name = kconfig::store::read::RequiredString("merged.text.name");

        ki18n::LoadLanguage("i18n", language);
        const std::string preview = ki18n::Get("demo.ready");

        std::cout << std::boolalpha
                  << "Core i18n demo: enabled=" << enabled
                  << ", port=" << port
                  << ", language=" << language
                  << ", name=" << name
                  << ", preview=" << preview << "\n";
    } catch (const std::exception& ex) {
        std::cerr << "Core i18n demo failed: " << ex.what() << "\n";
        return 1;
    }

    ki18n::demo::alpha::EmitDemoOutput();

    std::cout << "KI18N demo core compile/link/integration check passed\n";
    return 0;
}
