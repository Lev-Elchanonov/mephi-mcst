#pragma once
#include "ir_structs.hpp"
#include <unordered_map>


namespace ir {
    class cfg_builder {
    public:
        void build(function& f) {
            f.blocks_.clear();
            f.entry_ = nullptr;

            std::vector<basic_block*> blocks;
            basic_block* current = nullptr;

            auto start_block = [&](const std::string& label) {
                auto b = std::make_unique<basic_block>();
                b->label_ = label;
                auto* raw_block = b.get();
                f.blocks_.push_back(std::move(b));
                blocks.push_back(raw_block);
                current = raw_block;
            };

            int implicit = 0;
            for (const auto& op : f.operations_) {
                if (op->code_ == op_code::LABEL) {
                    start_block(op->label_);
                    current->operations_.push_back(op);
                } else {
                    if (!current) {
                        start_block("L" + std::to_string(implicit++));
                    }
                    current->operations_.push_back(op);

                    // эти операции завершают базовый блок, поэтому текущий надо закрыть
                    if (op->code_ == op_code::JUMP ||
                        op->code_ == op_code::BRANCH ||
                        op->code_ == op_code::RET) {
                        current = nullptr;
                        }
                }
            }
            if (!blocks.empty()) f.entry_ = blocks.front();

            // соответствие лейблов блокам (1 метка = 1 блок)
            std::unordered_map<std::string, basic_block*> label_to_block;
            for (auto* b : blocks)
                label_to_block[b->label_] = b;

            // дальше добавляем ребра
            for (size_t i = 0; i < blocks.size(); ++i) {
                auto* block = blocks[i];
                if (block->operations_.empty()) continue;
                auto last = block->operations_.back();

                auto add_succ = [&](const std::string& lbl) {
                    auto it = label_to_block.find(lbl);
                    if (it != label_to_block.end()) {
                        block->successors_.push_back(it->second);
                        it->second->predecessors_.push_back(block);
                    }
                };
                if (last->code_ == op_code::JUMP) {
                    add_succ(last->label_);
                } else if (last->code_ == op_code::BRANCH) {
                    add_succ(last->true_label_);
                    add_succ(last->false_label_);
                } else if (last->code_ != op_code::RET) {
                    if (i + 1 < blocks.size()) {
                        block->successors_.push_back(blocks[i + 1]);
                        blocks[i + 1]->predecessors_.push_back(block);
                    }
                }
            }
        }

    };

}