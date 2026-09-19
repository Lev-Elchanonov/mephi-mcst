#include <iostream>
#include <memory>
#include <ostream>

#include "view.hpp"


int main() {
    try {
        auto graph_ = std::make_unique<graph::orgraph_t<std::string, std::string>>();
        auto dialogue = std::make_unique<view<graph::orgraph_t<std::string, std::string>>>((graph_.get()));
        dialogue->chat(std::cin);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
