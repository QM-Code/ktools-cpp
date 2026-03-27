#include <alpha/sdk.hpp>

#include <ki18n.hpp>

#include <iostream>

namespace {

bool g_initialized = false;

} // namespace

namespace ki18n::demo::alpha {

void Initialize() {
    if (!g_initialized) {
        g_initialized = true;
    }
}

void EmitDemoOutput() {
    Initialize();
    std::cout << "[alpha] demo sdk initialized\\n";
}

} // namespace ki18n::demo::alpha
