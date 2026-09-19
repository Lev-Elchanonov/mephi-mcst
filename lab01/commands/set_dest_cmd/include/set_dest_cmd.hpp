#pragma once
#include "i_command.hpp"

template < typename Tgraph >
class set_dest_cmd final : public i_command<Tgraph> {
public:
    std::string execute(Tgraph& graph, const std::vector<std::string>& args) override {
        if (!validate_args(args)) {
            return "Failure: Incorrect args";
        }
        auto edge = std::find_if(graph.ebegin(), graph.eend(), [&](auto& e) {
            return e.get_source()->get_data() == args[0] && e.get_dest()->get_data() == args[1];
        });
        auto new_node = std::find_if(graph.nbegin(), graph.nend(), [&](auto& n) {
            return n.get_data() == args[2];
        });
        graph.set_dest(edge, new_node);
        return "";
    }


    bool validate_args(const std::vector<std::string>& args) override {
        return args.size() == 3;
    }
};
