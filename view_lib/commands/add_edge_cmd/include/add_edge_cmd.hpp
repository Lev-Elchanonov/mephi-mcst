#pragma once

#include "i_command.hpp"

template <class Tgraph>
class add_edge_cmd final : public i_command<Tgraph> {
public:
    std::string execute(Tgraph& graph, const std::vector<std::string>& args) override {
        if (!validate_args(args)) {
            return "Failure: Incorrect args";
        }
        auto source_it = std::find_if(graph.nbegin(), graph.nend(), [&](const auto& node) {
            return node.get_data() == args[0];
        });
        auto dest_it = std::find_if(graph.nbegin(), graph.nend(), [&](const auto& node) {
            return node.get_data() == args[1];
        });

        graph.add_edge(source_it, dest_it, args[2]);
        return "";
    }

    bool validate_args(const std::vector<std::string>& args) override {
        bool broken_arguments = args.size() != 3 || std::isdigit(args[0].front()) || std::isdigit(args[1].front());
        return !broken_arguments;
    }
};
