#pragma once
#include "i_command.hpp"

template < typename Tgraph >
class clear_cmd final : public i_command<Tgraph> {
public:
    std::string execute(Tgraph& graph, const std::vector<std::string>& args) override {
        if (!validate_args(args)) {
            return "Failure: Incorrect args";
        }
        graph.clear();
        return "";
    }


    bool validate_args(const std::vector<std::string>& args) override {
        return args.empty();
    }
};
