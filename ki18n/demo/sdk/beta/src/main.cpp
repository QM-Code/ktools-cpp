#include <beta/sdk.hpp>

#include <ki18n.hpp>

#include <iostream>

namespace {

bool g_initialized = false;

} // namespace

namespace ki18n::demo::beta {

void Initialize() {
    if (!g_initialized) {
        g_initialized = true;
    }
}

void EmitDemoOutput() {
    Initialize();
    std::cout << "[beta] demo sdk initialized\\n";
}

} // namespace ki18n::demo::beta
