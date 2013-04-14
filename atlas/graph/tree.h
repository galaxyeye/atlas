/*
 * directed_tree.h
 *
 *  Created on: Apr 14, 2013
 *      Author: vincent
 */

#ifndef ATLAS_DIRECTED_TREE_H_
#define ATLAS_DIRECTED_TREE_H_

#include <memory>

#include <boost/mpl/at.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/fusion/container/map.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/support/pair.hpp>
#include <boost/graph/visitors.hpp>

namespace atlas {

  template<typename Processor, typename... NodeList>
  struct processor_wrapper {

    processor_wrapper(Processor& processor) : processor(processor) {}

    void operator()(NodeList& nodes...) {
      processor()(nodes...);
    }

    Processor& processor;
  };

  // visit the node for the first time
  template<typename Node, typename Processor, typename Event>
  struct tree_node_visitor : public boost::base_visitor<tree_node_visitor> {

    typedef Event event_filter;

    tree_node_visitor(Node* vertexes) : vertexes(vertexes) {}

    template<class Vertex, class Graph>
    inline void operator()(Vertex current, Graph& g) {
      Processor processor(vertexes[current]);
      processor();
    }

    Node* vertexes;
  };

  template<typename Node, typename Processor, typename Event>
  struct tree_edge_visitor : public boost::base_visitor<tree_edge_visitor> {

    typedef Event event_filter;

    tree_edge_visitor(Node* vertexes) : vertexes(vertexes) {}

    template<class Edge, class Graph>
    inline void operator()(Edge e, Graph& g) {
      Processor processor(vertexes[target(current, g), source(current, g)]);
      processor();
    }

    Node* vertexes;
  };

  template<typename Node, typename Processor>
  using tree_node_initializer = tree_node_visitor<boost::on_initialize_vertex, Processor>;

  template<typename Node, typename Processor>
  using tree_node_starter = tree_node_visitor<boost::on_start_vertex, Processor>;

  template<typename Node, typename Processor>
  using tree_node_discover = tree_node_visitor<boost::on_discover_vertex, Processor>;

  template<typename Node, typename Processor>
  using tree_node_examiner = tree_node_visitor<boost::on_examine_vertex, Processor>;

  template<typename Node, typename Processor>
  using tree_node_finisher = tree_node_visitor<boost::on_finish_vertex, Processor>;

  template<typename Node, typename Processor>
  using tree_edge_examiner = tree_edge_visitor<boost::on_examine_edge, Processor>;

  template<typename Node, typename Processor>
  using tree_edge_finisher = tree_edge_visitor<boost::on_tree_edge, Processor>;

  template<typename Node, typename Processor>
  using tree_edge_back_examiner = tree_edge_visitor<boost::on_back_edge, Processor>;

  namespace {

    template<typename TreeNode>
    struct __processor_invoker {

      __processor_invoker(TreeNode* current, TreeNode* parent) : current(current), parent(parent) {}

      template<typename TypeTreeNode>
      void operator()(TypeTreeNode& pair) const {
        typedef typename boost::fusion::result_of::first<TypeTreeNode>::type first_type;

        // TODO : bad
        if (typeid(*current) == typeid(first_type)) {
          if (pair.second) pair.second(current, parent);
        }
      }

      TreeNode* current;
      TreeNode* parent;
    };

    template<typename TreeNode>
    struct __sub_processor_tree_applier {

      __sub_processor_tree_applier(TreeNode* current, TreeNode* parent) : current(current), parent(parent) {}

      template<typename MetaNode>
      void operator()(MetaNode& node) const {
        using boost::fusion::result_of::first;
        using boost::fusion::for_each;

        typedef typename first<MetaNode>::type first_type;

        if (typeid(*parent) == typeid(first_type)) {
          __processor_invoker invoker(current, parent);
          for_each(node.second, invoker);
        }
      }

      TreeNode* current;
      TreeNode* parent;
    };

  } // anonymous

  // TODO : inefficiency : must visit 6 nodes in TypeTree each time where 2~4 visits are wasted which
  // can not be optimized at runtime
  // a better way is to generate empty function calls for useless check path at compiler time
  // which can be optimized at runtime and RTTI can be avoid
  template<typename TreeNode, typename TypeTree>
  struct processor_invoker {

    processor_invoker(TreeNode* current, TreeNode* parent = current) : current(current), parent(parent) {}

    void operator()() {
      using boost::fusion::for_each;

      static const TypeTree mtree;
      __sub_processor_tree_applier applier(current, parent);
      for_each(mtree, applier);
    }

    TreeNode* current;
    TreeNode* parent;
  };
} // atlas

#endif /* DIRECTED_TREE_H_ */
