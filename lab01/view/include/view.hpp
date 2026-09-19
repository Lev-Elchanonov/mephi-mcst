#pragma once
#include <iostream>

#include "controller.hpp"

// Класс-интерфейс для работы с программой
// Принимает команды от пользователя и передает их в контроллер
// Использовался паттерн MVC
template < typename Tgraph >
class view {
private:
    controller<Tgraph> controller_;
public:
    explicit view(Tgraph* graph) : controller_(graph) {}


    void chat(std::istream& istr) {
        std::string input;
        std::string message;
        while (std::getline(istr, input)) {
            if (input == "exit") { break; }
            std::string res_msg = controller_.process_command(input);
            if (!res_msg.empty()) {
                std::cout << res_msg << std::endl;
            }
        }
    }
};
