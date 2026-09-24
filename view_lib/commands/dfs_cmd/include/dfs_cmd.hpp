#pragma once

#include <unordered_set>

#include "i_command.hpp"

template < typename Tgraph >
class dfs_cmd final : public i_command<Tgraph> {
public:
    bool validate_args(const std::vector<std::string>& args) override {
        return args.size() == 1;
    }


    std::string execute(Tgraph& graph, const std::vector<std::string>& args) override {
        if (!validate_args(args)) {
            return "Failure: Incorrect args";
        }

        auto& nodes = graph.get_nodes();
        auto start = std::find_if(nodes.begin(), nodes.end(),
            [&](const auto& node) {
                return node.get_data() == args[0];
            });

        if (start == nodes.end()) {
            return "Failure: start node not found";
        }

        auto order = graph.dfs(start);
        std::string res;
        for (auto& node : order) {
            res += node->get_data() + " ";
        }
        res.pop_back();
        return res;
    }

};
