#include <stdio.h>
#include <algorithm>
#include <sstream>

#include "json_graph_tool.h"
#include "json_writer.h"

using namespace std;

vector<Edge*> FindNonPhonyDeps(Edge* edge);
void FindNonPhonyDepsRecursive(Edge* edge, vector<Edge*> &result, set<Edge*> &added);

vector<Edge*> FindNonPhonyDepsFromNode(Node* from);
void FindNonPhonyDepsFromNodeRecursive(Node* from, vector<Edge*>& result, set<Edge*> &added);

void WriteRootTarget(JsonWriter *jw, Node* target, map<Edge*, int> &edge_ids, bool is_first);
void CollectNonPhonyInputs(Node* candidate_input, vector<Node*>& inputs, set<Node*>& visited);

void WriteEdge(JsonWriter *jw, Edge* edge, map<Edge*, int> &edge_ids, bool is_first);
void WriteEdgeReference(JsonWriter *jw, Edge* edge, int ref_id, bool is_first);

void WriteAllInputs(JsonWriter *jw, Edge *edge);
void WriteSourceInputs(JsonWriter *jw, Edge *edge);
void WriteOutputs(JsonWriter *jw, Edge *edge);

void JsonGraphTool::AddRootTarget(Node* node) {
  root_nodes_.push_back(node);
  
  if(!edge_idxs_.count(node->in_edge())) {
      visited_edges_.clear();
      TopoAddEdge(node->in_edge());
  }
}

/// Add a given edge and its dependencies in the edges_ vector, in topological order.
/// If a dependency was already in the edges_ vector we don't add it twice (the topo order is still preserved) 
int JsonGraphTool::TopoAddEdge(Edge* edge) {
  if(edge_idxs_.count(edge)) { // Already in the list
    return 0; 
  } 

  if(visited_edges_.find(edge) != visited_edges_.end()) {
    return 1; // Loop
  }

  vector<Edge*> deps = FindNonPhonyDeps(edge);
  
  visited_edges_.insert(edge);
  for (vector<Edge*>::iterator d = deps.begin(); d != deps.end(); ++d) {
    if(TopoAddEdge(*d)) {
      return 1;             // Bubble failure up
    }
  }
  visited_edges_.erase(edge);

  edge_idxs_[edge] = ++last_edge_idx_;
  edges_.push_back(edge);
  
  return 0; // Success
}




 
/// Find all the inputs of an Edge resolving the phony renames.
/// For example, given the edge E with nodes n1 and p_n01, in which
/// p_n01 is a phony rename of n0 and n1  
///
///    n0---.--> {phony} --> p_n01-----.---> {E}
///    n1--´                          /                  
///                             n2---´
///  
/// Then FindNonPhonyDeps(E) puts {n0,n1,n2} in result.
vector<Edge*> FindNonPhonyDeps(Edge* edge) {
  vector<Edge*> result;
  set<Edge*> added;
  FindNonPhonyDepsRecursive(edge, result, added);
  return result;
}


void FindNonPhonyDepsRecursive(Edge* edge, vector<Edge*> &result, set<Edge*> &added) {
  for (vector<Node*>::iterator i = edge->inputs_.begin(); i != edge->inputs_.end(); ++i) {
      FindNonPhonyDepsFromNodeRecursive(*i, result, added);
  }
}

 /// Given a node, determine all direct dependencies (commands) which are not a result
 /// of a phony edge (effectively removing all the phony rule renamings).
 /// These dependencies are added to the result vector passed as a parameter.
vector<Edge*> FindNonPhonyDepsFromNode(Node* from) {
  vector<Edge*> result;
  set<Edge*> added;
  FindNonPhonyDepsFromNodeRecursive(from, result, added);
  return result;
} 


void FindNonPhonyDepsFromNodeRecursive(Node* from, vector<Edge*>& result, set<Edge*>& added) {
  Edge* in = from->in_edge();
  if(!in || added.count(in) > 0) {
    // Either 'from' is a source node or we already added the dependency
    return;
  }

  added.insert(in);
  if (in->is_phony()) {
    // Keep looking
    FindNonPhonyDepsRecursive(in, result, added);
  } else {
    result.push_back(in);
  }
} 

void JsonGraphTool::Flush() {
  JsonWriter jw = JsonWriter(stdout);
  jw.StartObject(false);
  jw.StartObjectProperty("Graph", false);
  jw.StartArrayProperty("Nodes", false);

  // edges_ has all edges (BXL Nodes) in topological order
  for (vector<Edge*>::iterator e = edges_.begin(); e != edges_.end(); ++e) {
    WriteEdge(&jw, *e, edge_idxs_, e == edges_.begin());
  }
  jw.EndArray();

  jw.StartArrayProperty("Targets", true);
  for(vector<Node*>::iterator t = root_nodes_.begin(); t != root_nodes_.end(); ++t) {
    WriteRootTarget(&jw, *t, edge_idxs_, t == root_nodes_.begin());
  }
  jw.EndArray();
  jw.EndObject();

  // The serialization was a success
  jw.StringProperty("FailureReason", "", true);
  
  jw.EndObject();
}

void WriteEdgeReference(JsonWriter *jw, Edge* edge, int ref_id, bool is_first) {
    jw->StartObject(!is_first);
    jw->NumericalStringProperty("$ref", ref_id, false);
  //  jw->StringProperty("rule", edge->rule().name(), true);
    jw->EndObject();
}


// TODO: This would write only the source file inputs. Maybe it doesn't make sense
void WriteSourceInputs(JsonWriter *jw, Edge *edge) {
  vector<Node*> true_inputs;
  set<Node*> visited_inputs;

  for (vector<Node*>::iterator in = edge->inputs_.begin(); in != edge->inputs_.end(); ++in) {
      CollectNonPhonyInputs(*in, true_inputs, visited_inputs);
  }
  
  for(vector<Node*>::iterator in = true_inputs.begin(); in != true_inputs.end(); ++in) {
    jw->String((*in)->path(), in != true_inputs.begin());    
  }
}



///  Write a comma separated list of all the 'true' inputs for this edge (resolving phony renames).
///  If in_edge isn't phony then it is a 'true' input, so we add it
///  If in_edge is phony: 
///     collect its inputs and recursively add them
void WriteAllInputs(JsonWriter *jw, Edge *edge) {
  vector<Node*> true_inputs;
  set<Node*> visited_inputs;

  for (vector<Node*>::iterator in = edge->inputs_.begin(); in != edge->inputs_.end(); ++in) {
      CollectNonPhonyInputs(*in, true_inputs, visited_inputs);
  }
  
  for(vector<Node*>::iterator in = true_inputs.begin(); in != true_inputs.end(); ++in) {
    jw->String((*in)->path(), in != true_inputs.begin());    
  }
}

void CollectNonPhonyInputs(Node* candidate_input, vector<Node*>& inputs, set<Node*>& visited) {
  if(visited.count(candidate_input) > 0) {
    return;
  }
  Edge* in_edge = candidate_input->in_edge();
  if (!(in_edge && in_edge->is_phony())) { 
    // "True" input file, not coming from phony
    inputs.push_back(candidate_input);
    visited.insert(candidate_input);
  } else {
      // Phony edge, so that node is an alias for many other nodes
      // Those nodes are the true inputs, but some may be phonies as well
      // so recursively call this funcion.
      for (vector<Node*>::iterator in = in_edge->inputs_.begin(); in != in_edge->inputs_.end(); ++in) {
        CollectNonPhonyInputs(*in, inputs, visited);
      }
  }
}



/// Write a comma separated list of all the outputs of the edge.
void WriteOutputs(JsonWriter *jw, Edge *edge) {
  if(edge->is_phony()) {  
    // The only phonies that we add are the ones that produce the root targets.
    // We are not interested in the 'output' which is just an alias. 
    return;
  }

  for (vector<Node*>::iterator out = edge->outputs_.begin(); out != edge->outputs_.end(); ++out) {
    jw->String((*out)->path(), out != edge->outputs_.begin());    
  }

  // If there is a depfile declared, we add it as an output
  string depfile = edge->GetBinding("depfile");
  if(!depfile.empty()) {
    jw->String(depfile, !edge->outputs_.empty());
  }
}


/// Write an edge into the JsonWriter writer  
void WriteEdge(JsonWriter *jw, Edge* edge, map<Edge*, int> &edge_ids, bool is_first) {
  jw->StartObject(!is_first); 
  int edge_id = edge_ids[edge];
  jw->NumericalStringProperty("$id", edge_id, false);
  jw->StringProperty("rule", edge->rule().name(), true);
  jw->StringProperty("command", edge->EvaluateCommand(), true);
  
  // Dependencies array
  jw->StartArrayProperty("dependencies", true);

  vector<Edge*> deps = FindNonPhonyDeps(edge);

  for(vector<Edge*>::iterator dep = deps.begin(); dep != deps.end(); dep++) {
    if(edge_ids.find(*dep) == edge_ids.end()) { // TODO: Better error handling
      fprintf(stderr, "Unknown edge");
      exit(1);
    }

    int ref_id = edge_ids[*dep];
    if(ref_id > edge_id) {                  // This should never happen, but be defensive
      fprintf(stderr, "Wrong topo-sort");   // TODO: Better error handling
      exit(1);
    }
    WriteEdgeReference(jw, *dep, ref_id, dep == deps.begin());
  }
  jw->EndArray();


  jw->StartArrayProperty("inputs", true);
  WriteAllInputs(jw, edge);
  jw->EndArray();

  jw->StartArrayProperty("outputs", true);
  WriteOutputs(jw, edge);
  jw->EndArray();
  string rspfile = edge->GetUnescapedRspfile();
  if(!rspfile.empty()) {
    string content = edge->GetBinding("rspfile_content");

    jw->StartObjectProperty("responseFile", true);
    jw->StringProperty("name", rspfile, false);
    jw->StringProperty("content", content, true);
    jw->EndObject();
  }
    jw->EndObject();

}


/// Write an object with the information of a root target  
void WriteRootTarget(JsonWriter *jw, Node* target, map<Edge*, int> &edge_ids, bool is_first) {
  jw->StartObject(!is_first);
  jw->StringProperty("name", target->path(), false);
  
  /*
    TODO: For now we are keeping the final in_edge of the root node, even if it's phony
          We could instead collect all the non-phony edges and write a producer_nodes array
          instead of having a single producer_node, like so:  

    vector<Edge*> producers = FindNonPhonyDepsFromNode(target);
    jw->StartArrayProperty("producer_nodes", true);
    for(vector<Edge*>::iterator p = producers.begin(); p != producers.end(); p++) {
      WriteEdgeReference(jw, *p, edge_ids[*p], p == deps.begin());
  } 
  */
  jw->StartObjectProperty("producer_node", true);
  jw->NumericalStringProperty("$ref", edge_ids[target->in_edge()], false);

  jw->EndObject();
  jw->EndObject();
}