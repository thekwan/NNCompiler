#include <iostream>
#include <fstream>
#include <queue>
#include <stack>

#include "Node.h"

using namespace std;
using namespace nnframework;

Node::Node(string name, string type) {
    _name = name;
    _type = type;
}

Node::~Node(void) {
    _successor.clear();
    _predecessor.clear();
}

string Node::get_name(void) {
    return _name;
}

string Node::get_type(void) {
    return _type;
}

string Node::get_indegree(void) {
    return _predecessor.size();
}

string Node::get_outdegree(void) {
    return _successor.size();
}

vector<shared_ptr<Node>> Node::get_predecessor(void) {
    return _predecessor;
}

vector<shared_ptr<Node>> Node::get_successor(void) {
    return _successor;
}

shared_ptr<Node> Node::get_predecessor(int i) {
    return _predecessor[i];
}

shared_ptr<Node> Node::get_successor(void) {
    return _successor[i];
}

void Node::add_predecessor(shared_ptr<Node> np) {
    _predecessor.push_back( np );
}

void Node::add_successor(shared_ptr<Node> np) {
    _successor.push_back( np );
}

void Node::set_predecessor(vector<shared_ptr<Node>> np_list) {
    _predecessor.clear();
    _predecessor = np_list;
}

void Node::set_successor(vector<shared_ptr<Node>> np_list) {
    _successor.clear();
    _successor = np_list;
}
