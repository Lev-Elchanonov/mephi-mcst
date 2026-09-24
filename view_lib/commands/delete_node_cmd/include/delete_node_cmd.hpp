#pragma once
#include "i_command.hpp"

template < typename Tgraph >
class delete_node_cmd final : public i_command<Tgraph> {
public:
    std::string execute(Tgraph& graph, const std::vector<std::string>& args) override {
        if (!validate_args(args)) {
            return "Failure: Incorrect args";
        }
        auto iter = std::find_if(graph.nbegin(), graph.nend(),
            [&](const auto& node) {
                return node.get_data() == args[0];
            });
        graph.delete_node(iter);
        return "";
    }

    bool validate_args(const std::vector<std::string>& args) override {
        bool broken_arguments = args.size() != 1 || std::isdigit(args[0].front());
        return !broken_arguments;
    }
};