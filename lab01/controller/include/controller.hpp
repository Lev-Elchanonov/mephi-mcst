#pragma once
#include <memory>
#include <regex>
#include <unordered_map>
#include <vector>


#include "i_command.hpp"


#include "add_edge_cmd.hpp"
#include "add_node_cmd.hpp"
#include "delete_edge_cmd.hpp"
#include "delete_node_cmd.hpp"
#include "visualize_cmd.hpp"
#include "set_source_cmd.hpp"
#include "set_dest_cmd.hpp"
#include "clear_cmd.hpp"
#include "dfs_cmd.hpp"

// Парсит команды, переданные от View, и выбирает одну из команд
template < typename Tgraph>
class controller {
private:
    Tgraph* graph_;                                                          // граф
    std::unordered_map<std::string, std::unique_ptr<i_command<Tgraph>>> commands_;  // хеш-мапа команд (паттерн Command)


    static std::vector<std::string> split_tokens(std::string str) { // приватная функция парсинга текста пользователя на токены
        std::regex reg(R"(\s+)");
        std::sregex_token_iterator iter(str.begin(), str.end(), reg, -1);
        std::sregex_token_iterator end;

        std::vector<std::string> tokens(iter, end);
        return tokens;
    }

    void register_default_commands() {  // для регистрации стандартных команд, прописанных в задании
        commands_.emplace("NODE", std::make_unique<add_node_cmd<Tgraph>>());
        commands_.emplace("EDGE", std::make_unique<add_edge_cmd<Tgraph>>());
        commands_.emplace("REMOVE NODE", std::make_unique<delete_node_cmd<Tgraph>>());
        commands_.emplace("REMOVE EDGE", std::make_unique<delete_edge_cmd<Tgraph>>());
        commands_.emplace("SET SOURCE", std::make_unique<set_source_cmd<Tgraph>>());
        commands_.emplace("SET DEST", std::make_unique<set_dest_cmd<Tgraph>>());
        commands_.emplace("LOOK", std::make_unique<visualize_cmd<Tgraph>>());
        commands_.emplace("DFS", std::make_unique<dfs_cmd<Tgraph>>());
        commands_.emplace("CLEAR", std::make_unique<clear_cmd<Tgraph>>());
    }

public:
    explicit controller(Tgraph* graph) {
        if (!graph) {
            throw std::invalid_argument("Graph cannot be nullptr\n");
        }
        graph_ = graph;
        register_default_commands();
    }

    std::string process_command(const std::string& command) {  // поиск и обработка команды
        std::vector<std::string> tokens = split_tokens(command);        // первый полученный токен - название команды, остальные - ее аргументы
        std::string command_name = tokens.front();
        tokens.erase(tokens.begin());
        if (command_name == "REMOVE" || command_name == "SET") { // для REMOVE в имя команды входят первые 2 токена (REMOVE NODE, REMOVE EDGE)
            command_name += " " + tokens.front();
            tokens.erase(tokens.begin());
        }
        auto cmd = commands_.find(command_name); // поиск команды в хеш-мапе
        if (cmd == commands_.end()) {
            return  "";
        }
        return cmd->second->execute(*graph_, tokens); // вызов override метода в наследнике, реализующем конкретную команду
    }

    void add_command(const std::string& command, std::unique_ptr<i_command<Tgraph>> command_ptr) { // для добавления новой команды в хеш-мапу (для расширяемости)
        commands_.emplace(command, std::move(command_ptr));
    }

    void delete_command(const std::string& command) {    // удаление команды из хеш-мапы
        commands_.erase(command);
    }
};
