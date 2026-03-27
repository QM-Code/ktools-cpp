#include <alpha/sdk.hpp>
#include <beta/sdk.hpp>
#include <gamma/sdk.hpp>
#include <ki18n.hpp>
#include <kconfig.hpp>
#include <kconfig/store.hpp>
#include <kcli.hpp>
#include <ktrace.hpp>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#define KTRACE(channel, ...) tracer.trace(channel, __VA_ARGS__)

int main(int argc, char** argv) {
    ktrace::Logger logger;

    ktrace::TraceLogger tracer("ki18n_demo_omega");
    tracer.addChannel("store");
    tracer.addChannel("store.requests");
    tracer.addChannel("asset");
    tracer.addChannel("asset.requests");

    logger.addTraceLogger(tracer);
    logger.addTraceLogger(kconfig::GetTraceLogger());
    logger.addTraceLogger(ki18n::GetTraceLogger());

    kcli::Parser parser;
    parser.addInlineParser(logger.makeInlineParser(tracer));
    parser.addInlineParser(kconfig::cli::GetInlineParser());

    parser.parseOrExit(argc, argv);

    const std::filesystem::path repoRoot = std::filesystem::current_path();
    const std::filesystem::path runtimeRoot = repoRoot / "demo" / "exe" / "omega" / "runtime";
    const std::filesystem::path defaultsPath = runtimeRoot / "defaults.json";
    const std::filesystem::path userPath = runtimeRoot / "user.json";
    const std::filesystem::path sessionPath = runtimeRoot / "session.json";
    const std::filesystem::path i18nPath = runtimeRoot / "i18n.json";

    KTRACE("store.requests",
           "omega demo preparing config load defaults='{}' user='{}' session='{}' i18n='{}'",
           defaultsPath.string(),
           userPath.string(),
           sessionPath.string(),
           i18nPath.string());

    std::string ioError;

    KTRACE("store.requests", "store::fs::LoadReadOnly('defaults', '{}')", defaultsPath.string());
    if (!kconfig::store::fs::LoadReadOnly("defaults", defaultsPath, &ioError)) {
        std::cerr << "Failed to load defaults config: " << ioError << "\n";
        return 1;
    }

    if (const char* userConfigDirname = std::getenv("KI18N_DEMO_USER_CONFIG_DIRNAME");
        userConfigDirname != nullptr && *userConfigDirname != '\0') {
        KTRACE("store.requests", "store::user::SetDirname('{}')", userConfigDirname);
        if (!kconfig::store::user::SetDirname(userConfigDirname)) {
            std::cerr << "Failed to set user config dirname: " << userConfigDirname << "\n";
            return 1;
        }

        if (!kconfig::store::user::HasConfigFile()) {
            ioError.clear();
            KTRACE("store.requests", "store::user::InitializeConfigFile(object)");
            if (!kconfig::store::user::InitializeConfigFile(kconfig::json::Object(), &ioError)) {
                std::cerr << "Failed to initialize user config file: " << ioError << "\n";
                return 1;
            }
        }

        kconfig::store::user::LoadOptions userLoadOptions{};
        userLoadOptions.mode = kconfig::store::user::LoadMode::Mutable;
        ioError.clear();
        KTRACE("store.requests", "store::user::LoadConfigFile('user', mode=Mutable)");
        if (!kconfig::store::user::LoadConfigFile("user", userLoadOptions, &ioError)) {
            std::cerr << "Failed to load user config file via user-config API: " << ioError << "\n";
            return 1;
        }
    } else {
        ioError.clear();
        KTRACE("store.requests", "store::fs::LoadMutable('user', '{}')", userPath.string());
        if (!kconfig::store::fs::LoadMutable("user", userPath, &ioError)) {
            std::cerr << "Failed to load user config: " << ioError << "\n";
            return 1;
        }
    }

    ioError.clear();
    KTRACE("store.requests", "store::fs::LoadReadOnly('session', '{}')", sessionPath.string());
    if (!kconfig::store::fs::LoadReadOnly("session", sessionPath, &ioError)) {
        std::cerr << "Failed to load session config: " << ioError << "\n";
        return 1;
    }

    ioError.clear();
    KTRACE("store.requests", "store::fs::LoadReadOnly('i18n', '{}')", i18nPath.string());
    if (!kconfig::store::fs::LoadReadOnly("i18n", i18nPath, &ioError)) {
        std::cerr << "Failed to load i18n config: " << ioError << "\n";
        return 1;
    }

    std::vector<std::string> mergeSources = {"defaults", "user", "session"};
    if (kconfig::store::Has("config")) {
        KTRACE("store.requests",
               "omega demo detected CLI namespace 'config' -> append to merge sources");
        mergeSources.push_back("config");
    }
    KTRACE("store.requests", "store::Merge('merged', dynamic-sources)");
    if (!kconfig::store::Merge("merged", mergeSources)) {
        std::cerr << "Failed to merge config namespaces: defaults + user + session -> merged\n";
        return 1;
    }

    const auto shouldRunBackingRoundTrip = []() {
        const char* env = std::getenv("KI18N_DEMO_TEST_BACKING");
        if (env == nullptr || *env == '\0') {
            return false;
        }
        std::string token(env);
        std::transform(token.begin(),
                       token.end(),
                       token.begin(),
                       [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        return token != "0" && token != "false" && token != "no" && token != "off";
    };

    if (shouldRunBackingRoundTrip()) {
        KTRACE("store.requests", "Backing roundtrip test enabled via KI18N_DEMO_TEST_BACKING");

        std::ifstream originalInput(userPath, std::ios::binary);
        if (!originalInput) {
            std::cerr << "Backing roundtrip failed: unable to read original user config file\n";
            return 1;
        }
        std::ostringstream originalBuffer;
        originalBuffer << originalInput.rdbuf();
        const std::string originalUserFileText = originalBuffer.str();

        const auto restoreUserFile = [&]() {
            std::ofstream restoreOut(userPath, std::ios::binary | std::ios::trunc);
            if (!restoreOut) {
                return false;
            }
            restoreOut << originalUserFileText;
            return static_cast<bool>(restoreOut);
        };

        auto failBacking = [&](const std::string& message) {
            std::cerr << "Backing roundtrip failed: " << message << "\n";
            if (!restoreUserFile()) {
                std::cerr << "Backing roundtrip failed: unable to restore original user config file\n";
            }
            std::string reloadError;
            (void)kconfig::store::fs::ReloadBackingFile("user", &reloadError);
            return 1;
        };

        const std::string sentinel = "backing-write-sentinel";
        KTRACE("store.requests", "store::Set('user', 'text.name', '{}')", sentinel);
        if (!kconfig::store::Set("user", "text.name", sentinel)) {
            return failBacking("store::Set('user', 'text.name', sentinel) returned false");
        }
        KTRACE("store.requests", "store::fs::SetSaveIntervalSeconds('user', 0.0)");
        if (!kconfig::store::fs::SetSaveIntervalSeconds("user", 0.0)) {
            return failBacking("store::fs::SetSaveIntervalSeconds('user', 0.0) returned false");
        }
        ioError.clear();
        KTRACE("store.requests", "store::fs::FlushPendingWrites()");
        if (!kconfig::store::fs::FlushPendingWrites(&ioError)) {
            return failBacking("store::fs::FlushPendingWrites() returned false: " + ioError);
        }
        KTRACE("store.requests", "store::Set('user', 'text.name', 'memory-only-temp')");
        if (!kconfig::store::Set("user", "text.name", "memory-only-temp")) {
            return failBacking("store::Set('user', 'text.name', memory-only-temp) returned false");
        }
        ioError.clear();
        KTRACE("store.requests", "store::fs::ReloadBackingFile('user')");
        if (!kconfig::store::fs::ReloadBackingFile("user", &ioError)) {
            return failBacking("store::fs::ReloadBackingFile('user') returned false: " + ioError);
        }

        const auto roundTripName = kconfig::store::Get("user", "text.name");
        if (!roundTripName || !roundTripName->is_string() || roundTripName->get<std::string>() != sentinel) {
            return failBacking("reloaded user.text.name did not match sentinel");
        }

        if (!restoreUserFile()) {
            std::cerr << "Backing roundtrip failed: unable to restore original user config file\n";
            return 1;
        }
        ioError.clear();
        KTRACE("store.requests", "store::fs::ReloadBackingFile('user') after restore");
        if (!kconfig::store::fs::ReloadBackingFile("user", &ioError)) {
            std::cerr << "Backing roundtrip failed: store::fs::ReloadBackingFile after restore returned false: "
                      << ioError << "\n";
            return 1;
        }

        std::cout << "Backing file roundtrip check passed\n";
    }

    try {
        KTRACE("store.requests", "read::RequiredBool('merged.feature.enabled')");
        const bool enabled = kconfig::store::read::RequiredBool("merged.feature.enabled");
        KTRACE("store.requests", "read::RequiredUint16('merged.network.port')");
        const uint16_t port = kconfig::store::read::RequiredUint16("merged.network.port");
        KTRACE("store.requests", "read::RequiredPositiveUint16('merged.positive.u16')");
        const uint16_t retries = kconfig::store::read::RequiredPositiveUint16("merged.positive.u16");
        KTRACE("store.requests", "read::RequiredUint32('merged.numbers.u32')");
        const uint32_t maxSessions = kconfig::store::read::RequiredUint32("merged.numbers.u32");
        KTRACE("store.requests", "read::RequiredFloat('merged.limits.timeout')");
        const float timeout = kconfig::store::read::RequiredFloat("merged.limits.timeout");
        KTRACE("store.requests", "read::RequiredPositiveFiniteFloat('merged.positive.float')");
        const float positiveScale = kconfig::store::read::RequiredPositiveFiniteFloat("merged.positive.float");
        KTRACE("store.requests", "read::RequiredNonEmptyString('merged.client.Language')");
        const std::string language = kconfig::store::read::RequiredNonEmptyString("merged.client.Language");
        KTRACE("store.requests", "read::RequiredString('merged.text.name')");
        const std::string name = kconfig::store::read::RequiredString("merged.text.name");
        KTRACE("store.requests", "read::RequiredFloatArray('merged.values.weights')");
        const std::vector<float> weights = kconfig::store::read::RequiredFloatArray("merged.values.weights");
        KTRACE("asset.requests", "asset::LoadRequiredText('defaults', 'assets.banner')");
        std::string banner = kconfig::asset::LoadRequiredText("defaults", "assets.banner");
        while (!banner.empty() && (banner.back() == '\n' || banner.back() == '\r')) {
            banner.pop_back();
        }

        KTRACE("store.requests", "ki18n::LoadLanguage('i18n', '{}')", language);
        ki18n::LoadLanguage("i18n", language);
        const std::string preview = ki18n::Get("demo.ready");

        std::cout << std::boolalpha
                  << "ReadRequired demo: enabled=" << enabled
                  << ", port=" << port
                  << ", retries=" << retries
                  << ", maxSessions=" << maxSessions
                  << ", timeout=" << timeout
                  << ", scale=" << positiveScale
                  << ", language=" << language
                  << ", name=" << name
                  << ", banner=" << banner
                  << ", preview=" << preview
                  << ", weights=[";
        for (std::size_t i = 0; i < weights.size(); ++i) {
            if (i > 0) {
                std::cout << ", ";
            }
            std::cout << weights[i];
        }
        std::cout << "]\n";
    } catch (const std::exception& ex) {
        std::cerr << "ReadRequired demo failed: " << ex.what() << "\n";
        return 1;
    }

    ki18n::demo::alpha::EmitDemoOutput();
    ki18n::demo::beta::EmitDemoOutput();
    ki18n::demo::gamma::EmitDemoOutput();

    std::cout << "KI18N demo omega compile/link/integration check passed\n";
    return 0;
}
