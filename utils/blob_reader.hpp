#pragma once

#include <fstream>
#include <sstream>
#include <string>

std::string read_file(std::string path) {
    std::ifstream in(path);
    std::stringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}