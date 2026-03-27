#include <kcli.hpp>

#include <iostream>

int main(int argc, char** argv) {
    kcli::Parser parser;
    parser.parseOrExit(argc, argv);
    std::cout << "Bootstrap succeeded.\n";
    return 0;
}
