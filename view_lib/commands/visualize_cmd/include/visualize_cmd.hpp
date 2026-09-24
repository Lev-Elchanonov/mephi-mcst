#pragma once
#include <ctime>
#include <fstream>
#include <sstream>

#include "i_command.hpp"

template < typename Tgraph >
class visualize_cmd final : public i_command<Tgraph> {
public:
    std::string execute(Tgraph& graph, const std::vector<std::string>& args) override {
        if (!validate_args(args)) {
            return "Failure: Incorrect args";
        }
        std::ostringstream oss;
        graph.write_dot(oss,
            [](const auto& data) { return to_dot_label(data); },
            [](const auto& data) { return to_dot_label(data); });
        std::string dot_format = oss.str();


        std::string dot_filename = "/tmp/graph_" + std::to_string(std::time(nullptr)) + ".dot";
        std::string png_filename = "/tmp/graph_" + std::to_string(std::time(nullptr)) + ".png";

        std::ofstream dot_file(dot_filename);
        if (!dot_file) {
            return "Failure: Cannot create temporary DOT file";
        }
        dot_file << dot_format;
        dot_file.close();

        std::string command = "dot -Tpng " + dot_filename + " -o " + png_filename;
        int result = std::system(command.c_str());

        if (result != 0) {
            return "Failure: Graphviz conversion failed";
        }

        command = "xdg-open " + png_filename + " 2>/dev/null";

        result = std::system(command.c_str());

        if (result != 0) {
            return "Graph exported but cannot open viewer";
        }
        std::remove(dot_filename.c_str());
        return "";
    }

    bool validate_args(const std::vector<std::string>& args) override {
        return args.empty();
    }
    template <typename T>
    static std::string to_dot_label(const T& value) {
        if constexpr (std::is_void_v<T>) {
            return "";
        } else if constexpr (std::is_arithmetic_v<T>) {
            return std::to_string(value);
        } else if constexpr (std::is_convertible_v<T, std::string>) {
            return std::string(value);
        } else {
            return "?";
        }
    }

};
