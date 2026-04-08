#include <cstdio>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string_view>
#include "vm.hpp"
#include "debug.hpp"

static void repl() {
    std::string line;
    pegasus::VM vm{};
    while(true) {
        printf("> ");
        if(!std::getline(std::cin, line)) {
            printf("\n");
            break;
        } else {
            vm.interpret(line);
        }
    }
}

static void runFile(std::string_view path) {
    std::ifstream file{std::string{path}};

    if(!file.is_open()) {
        throw std::runtime_error("could not open file: " + std::string{path});
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::string source{buffer.str()};
    pegasus::VM vm{};

    pegasus::InterpretResult result{vm.interpret(source)};
    if(result == pegasus::InterpretResult::COMPILE_ERROR) exit(65);
    if(result == pegasus::InterpretResult::RUNTIME_ERROR) exit(70);
}

int main(int argc, const char* argv[]) {
    if(argc == 1) {
        repl();
    } else if(argc == 2) {
        try {
            runFile(argv[1]);
        } catch(const std::runtime_error& e) {
            fprintf(stderr, "error: %s\n", e.what());
            exit(74);
        }
    } else {
        fprintf(stderr, "usage: pegasus [path]\n");
        exit(64);
    }
    return 0;
}