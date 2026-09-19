#pragma once
#include <list>
#include <string>

#include <ostream>
#include <set>
#include <unordered_set>
#include <cassert>
#include <algorithm>

namespace graph {

    template < typename T >
    struct data_holder {
        T data_;
        template < typename ...Args>
        explicit data_holder(Args&&... args) : data_(std::forward<Args>(args)...) {}

        T& get_data() noexcept{ return data_; }
        const T& get_data() const noexcept { return data_; }

    };

    template <>
    struct data_holder<void> {
        template < typename ...Args >
        explicit data_holder(Args&&...) {}

        void get_data() noexcept {}
        void get_data() const noexcept {}
    };



    template < typename T >
    constexpr bool valid_data_v = std::is_void_v<T> || (std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T>);

    template < typename Tnode_data, typename Tedge_data,
               typename = std::enable_if_t<valid_data_v<Tnode_data> && valid_data_v<Tedge_data>>>
    class orgraph_t {
    public:
        class node_t;
        class edge_t;

        using node_iterator = typename std::list<node_t>::iterator;
        using const_node_iterator = typename std::list<node_t>::const_iterator;
        using edge_iterator = typename std::list<edge_t>::iterator;
        using const_edge_iterator = typename std::list<edge_t>::const_iterator;

        class node_t {
        private:
            friend class orgraph_t;

            data_holder<Tnode_data> data_;
            std::list<edge_iterator> incoming_edges_;
            std::list<edge_iterator> outgoing_edges_;


        public:
            template < typename ...Args >
            explicit node_t(Args&&... args) : data_(std::forward<Args>(args)...) {}

            auto& get_data() noexcept { return data_.get_data(); }
            auto& get_data() const noexcept { return data_.get_data(); }

            const std::list<edge_iterator>& incoming_edges() const noexcept { return incoming_edges_; }
            const std::list<edge_iterator>& outgoing_edges() const noexcept { return outgoing_edges_; }
        };

        class edge_t {
        private:
            friend class orgraph_t;

            data_holder<Tedge_data> data_;
            node_iterator source_;
            node_iterator dest_;
        public:
            template < typename ...Args >
            edge_t(node_iterator source, node_iterator dest, Args&&... args)
                : data_(std::forward<Args>(args)...), source_(source), dest_(dest) {}

            auto& get_data() noexcept { return data_.get_data(); }
            auto& get_data() const noexcept { return data_.get_data(); }

            node_iterator get_source() const noexcept { return source_; }
            node_iterator get_dest() const noexcept { return dest_; }

        };

    protected:
        std::list<node_t> nodes_;
        std::list<edge_t> edges_;


        // рекурсивный dfs с visited
        void dfs_impl(node_iterator node, std::unordered_set<const void*>& visited, std::vector<node_iterator>& order) const {
            const void* id = static_cast<const void*>(&*node);
            if ( visited.count(id) )
                return;
            visited.insert(id);

            order.push_back(node);

            for ( auto e : node->outgoing_edges() ) {
                dfs_impl(e->get_dest(), visited, order);
            }
        }
    public:
        virtual ~orgraph_t() = default;

        node_iterator nbegin() noexcept { return nodes_.begin(); }
        node_iterator nend() noexcept { return nodes_.end(); }

        const_node_iterator nbegin() const noexcept { return nodes_.begin(); }
        const_node_iterator nend() const noexcept { return nodes_.end(); }

        const_node_iterator ncbegin() const noexcept { return nodes_.cbegin(); }
        const_node_iterator ncend() const noexcept { return nodes_.cend(); }


        edge_iterator ebegin() noexcept { return edges_.begin(); }
        edge_iterator eend() noexcept { return edges_.end(); }

        const_edge_iterator ebegin() const noexcept { return edges_.begin(); }
        const_edge_iterator eend() const noexcept { return edges_.end(); }

        const_edge_iterator ecbegin() const noexcept { return edges_.cbegin(); }
        const_edge_iterator ecend() const noexcept { return edges_.cend(); }





        std::list<node_t>& get_nodes() noexcept { return nodes_; }
        const std::list<node_t>& get_nodes() const noexcept { return nodes_; }

        std::list<edge_t>& get_edges() noexcept { return edges_; }
        const std::list<edge_t>& get_edges() const noexcept { return edges_; }


        virtual void clear() noexcept { nodes_.clear(); edges_.clear(); }


        template < typename... Args >
        node_iterator add_node(Args&&... args) {
            if constexpr ( std::is_void_v<Tnode_data> ) { // если нет данных у нода
                nodes_.emplace_back(std::forward<Args>(args)...);
                auto it = std::prev(nodes_.end());

                orgraph_t::check_validation();
                return it;
            } else { // если есть
                node_t new_node( std::forward<Args>(args)... );
                auto existing = std::find_if(nodes_.begin(), nodes_.end(),
                    [&](const auto& node) {
                        return new_node.get_data() == node.get_data();
                    });
                if ( existing != nodes_.end() ) {
                    return this->nend();
                }
                nodes_.push_back(std::move(new_node));
                auto it = std::prev(nodes_.end());

                orgraph_t::check_validation();
                return it;
            }
        }

        template < typename... Args >
        edge_iterator add_edge(node_iterator source, node_iterator dest, Args&&... args) {
            // если невалидные итераторы подали
            if ( !contains_node(source) || !contains_node(dest) ) {
                return edges_.end();
            }
            // если такое ребро уже сущестует, то ничего не делаем, возвращаем итератор на конец
            auto existing = std::find_if(source->outgoing_edges().begin(), source->outgoing_edges().end(),
                [&](edge_iterator e) { return e->get_dest() == dest; });
            if ( existing != source->outgoing_edges().end() )
                return edges_.end();

            edges_.emplace_back(source, dest, std::forward<Args>(args)...);
            auto edge_iter = std::prev(edges_.end());

            source->outgoing_edges_.push_back(edge_iter);
            dest->incoming_edges_.push_back(edge_iter);



            check_validation();
            return edge_iter;
        }

        void set_source(edge_iterator edge, node_iterator new_src) {
            // проверка что нода и ребро существуют
            if ( !contains_node(new_src) || !contains_edge(edge) )
                return;

            edge->source_->outgoing_edges_.remove(edge);
            edge->source_ = new_src;
            new_src->outgoing_edges_.push_back(edge);


            check_validation();
        }

        void set_dest(edge_iterator edge, node_iterator new_dest) {
            // проверка что нода и ребро существуют
            if ( !contains_node(new_dest) || !contains_edge(edge) )
                return;


            edge->dest_->incoming_edges_.remove(edge);
            edge->dest_ = new_dest;
            new_dest->incoming_edges_.push_back(edge);


            check_validation();
        }

        void delete_edge(edge_iterator e) {
            // если такого ребра не существует, то ничего не делаем
            if ( !contains_edge(e) )
                return;

            e->source_->outgoing_edges_.remove(e);
            e->dest_->incoming_edges_.remove(e);
            edges_.erase(e);

            check_validation();
        }

        virtual void delete_node(node_iterator n) {
            // если такого нода не существует, то ничего не делаем
            if ( !contains_node(n) )
                return;
            // сначала удаляем инфу в каждом ребре об удаляемой ноде
            while ( !n->outgoing_edges_.empty() ) {
                delete_edge(n->outgoing_edges_.front());
            }
            while ( !n->incoming_edges_.empty() ) {
                delete_edge(n->incoming_edges_.front());
            }
            nodes_.erase(n);

            orgraph_t::check_validation();
        }

        template <typename T = Tnode_data>
        std::enable_if_t<!std::is_void_v<T>, node_iterator>
        find_node(const T& data) {
            return std::find_if(nodes_.begin(), nodes_.end(),
                [&](const node_t& n) { return n.get_data() == data; });
        }

        template <typename T = Tnode_data>
        std::enable_if_t<!std::is_void_v<T>, const_node_iterator>
        find_node(const T& data) const {
            return std::find_if(nodes_.begin(), nodes_.end(),
                [&](const node_t& n) { return n.get_data() == data; });
        }

        template < typename T = Tnode_data >
        std::enable_if_t<!std::is_void_v<T>, edge_iterator>
        find_edge(const T& source_data, const T& dest_data) {
            return std::find_if(edges_.begin(), edges_.end(),
                [&](const edge_t& e) { return e.get_source()->get_data() == source_data && e.get_dest()->get_data() == dest_data; });
        }

        template < typename T = Tnode_data >
        std::enable_if_t<!std::is_void_v<T>, const_edge_iterator>
        find_edge(const T& source_data, const T& dest_data) const {
            return std::find_if(edges_.begin(), edges_.end(),
                [&](const edge_t& e) { return e.get_source()->get_data() == source_data && e.get_dest()->get_data() == dest_data; });
        }


        // dfs оболчка, из которого будет вызываться рекурсивный dfs
        std::vector<node_iterator> dfs(node_iterator start_node) const noexcept {
            std::unordered_set<const void*> visited;
            visited.reserve(nodes_.size());
            std::vector<node_iterator> order;
            dfs_impl(start_node, visited, order);

            return order;
        }

        virtual void check_validation()  {
#ifdef DEBUG
            // проверка согласования дуг с нодами
            for ( auto node_it = nbegin(); node_it != nend(); ++node_it) {
                for ( auto e : node_it->incoming_edges() ) {
                    assert( e->get_dest() == node_it && "Error: incoming edge does not point to this node");
                }
                for ( auto e : node_it->outgoing_edges() ) {
                    assert( e->get_source() == node_it && "Error: outgoing edge does not point to this node");
                }
            }

            // проверка согласованности нодов с дугами
            for ( auto it = ebegin(); it != eend(); ++it) {
                auto src = it->get_source();
                auto dest = it->get_dest();

                auto& src_out = src->outgoing_edges();
                assert(std::find(src_out.begin(), src_out.end(), it) != src_out.end()
                        && "Error: edge not in source's outgoing list");

                auto& dest_in = dest->incoming_edges();
                assert(std::find(dest_in.begin(), dest_in.end(), it) != dest_in.end()
                        && "Error: edge not in dest's incoming list");
            }

#endif
        }

        template <typename Node_label, typename Edge_label>
        void write_dot(std::ostream& os, Node_label node_label, Edge_label edge_label) const {
            os << "digraph G {\n";
            for ( auto it = nbegin(); it != nend(); ++it ) {
                os << "  n" << &*it << " [label=\"" << node_label(it->get_data()) << "\"];\n";
            }
            for ( auto e = ebegin(); e != eend(); ++e ) {
                os << "  n" << &*e->get_source() << " -> n" << &*e->get_dest() << " [label=\"" << edge_label(e->get_data()) << "\"];\n";
            }
            os << "}\n";
        }

        bool contains_edge(edge_iterator e) {
            for (auto it = ebegin(); it != eend(); ++it) {
                if (it == e) return true;
            }
            return false;
        }
        bool contains_node(node_iterator n) {
            for (auto it = nbegin(); it != nend(); ++it) {
                if (it == n) return true;
            }
            return false;
        }
    };
}



namespace tree {
    template < typename Tnode_data, typename Tedge_data>
    class tree_t : public graph::orgraph_t<Tnode_data, Tedge_data> {
    public:
        using base_t = graph::orgraph_t<Tnode_data, Tedge_data>;
        using node_iterator = typename base_t::node_iterator;
        using edge_iterator = typename base_t::edge_iterator;
    private:
        node_iterator root_;
        bool has_root_;

        enum color {
            WHITE,
            GRAY,
            BLACK
        };
        bool has_loop() {
            if ( !has_root_ ){ return false; }
            std::unordered_map<const void*, color> colors;
            const void* id = static_cast<const void*>(&*root_);
            if ( colors[id] == WHITE ) {
                if ( has_loop_impl(root_, colors) ) {
                    return true;
                }
            }
            return false;
        }

        bool has_loop_impl(node_iterator n, std::unordered_map<const void*, color>& colors) {
            const void* id = static_cast<const void*>(&*n);
            colors[id] = GRAY;

            for ( auto e : n->outgoing_edges() ) {
                auto next = e->get_dest();
                const void* next_id = static_cast<const void*>(&*next);

                if ( colors[next_id] == GRAY ) { // есть backedge. Нашли цикл
                    return true;
                }
                if ( colors[next_id] == WHITE ) {
                    if ( has_loop_impl(next, colors) ) {
                        return true;
                    }
                }
            }
            colors[id] = BLACK; // обработанный
            return false;
        }
    public:
        void check_validation() override{
            base_t::check_validation();
#ifdef DEBUG
            if ( this->get_nodes().empty() && has_root_ == false )
                return;

            assert( has_root_ && "Error: tree must have root" );
            assert( !has_loop() && "Error: tree can not have loops" );

            assert( root_->incoming_edges().empty() && "Error: root can not have incoming edges");
#endif

        }

        tree_t(node_iterator root) : root_(root), has_root_(true) {}
        tree_t() : has_root_(false) {}
        node_iterator get_root() { return root_; }


        template < typename... Args >
        node_iterator add_node(Args&&... args) {
            auto it = base_t::add_node(std::forward<Args>(args)...);
            if ( has_root_ == false ) {
                root_ = this->nbegin();
                has_root_ = true;
            }

            check_validation();

            return it;
        }
        void clear() noexcept override {
            base_t::clear();
            has_root_ = false;
            root_ = this->nend();
        }

        void delete_node(node_iterator n) override{
            if (n == root_ && has_root_) {
                auto for_delete = this->dfs(root_);
                std::reverse(for_delete.begin(), for_delete.end());
                for (auto iter : for_delete)  {
                    base_t::delete_node(iter);
                }
                has_root_ = false;
                root_ = this->nend();
            } else {
                base_t::delete_node(n);
            }
        }


    };
}