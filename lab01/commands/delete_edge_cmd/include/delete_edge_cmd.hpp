#pragma once
#include "i_command.hpp"

template < typename Tgraph >
class delete_edge_cmd final : public i_command<Tgraph> {
public:
    std::string execute(Tgraph& graph, const std::vector<std::string>& args) override {
        if (!validate_args(args)) {
            return "Failure: Incorrect args";
        }
        // тут для отладки, поиск по данным, которые хранятся в edge
        auto& edges = graph.get_edges();
        auto& nodes = graph.get_nodes();
        auto node_source = std::find_if(nodes.begin(), nodes.end(),
            [&](const auto& node) {
                 return node.get_data() == args[0];
            });
        auto node_dest = std::find_if(nodes.begin(), nodes.end(),
            [&](const auto& node) {
                return node.get_data() == args[1];
            });

        auto edge = std::find_if(edges.begin(), edges.end(),
            [&](const auto& e) {
                return e.get_source() == node_source && e.get_dest() == node_dest;
            });
        graph.delete_edge(edge);
        return "";
    }

    bool validate_args(const std::vector<std::string>& args) override {
        bool broken_arguments = args.size() != 2 || std::isdigit(args[0].front()) || std::isdigit(args[1].front());
        return !broken_arguments;
    }
};
