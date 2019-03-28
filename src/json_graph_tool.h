#ifndef NINJA_JSON_GRAPH_TOOL_H_
#define NINJA_JSON_GRAPH_TOOL_H_

#include <set>
#include <map>
#include <vector>
#include "graph.h"

struct Node;
struct Edge;

/// Outputs the dependency graph in JSON format.
/// Phony edges are removed, keeping explicit dependencies.
struct JsonGraphTool {
  
  /// Add a Node as a build target. 
  /// This node can be the result of a phony edge or not.
  void JsonGraphTool::AddRootTarget(Node* node);
 
  /// Flush the JSON representation to standard output
  void Flush();

  JsonGraphTool::JsonGraphTool(std::string build_dir) {
    last_edge_idx_ = 0;
    build_dir_ = build_dir;
  }

  private:
    int TopoAddEdge(Edge* edge);
    void AddTarget(Node* node);

    std::vector<Node*> root_nodes_;
    std::map<Edge*, int> edge_idxs_;

    /// Set for quick lookup
    std::set<Edge*> visited_edges_; 

    // Vector for determinism in order (set is ordered by ptr)
    std::vector<Edge*> edges_; 
    
    // Keep count of references
    int last_edge_idx_;

    std::string build_dir_;
};

#endif  // NINJA_JSON_GRAPH_TOOL_H_
