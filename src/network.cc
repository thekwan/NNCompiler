#include <iostream>
#include <fstream>
#include <queue>
#include <stack>

#include "layer.hpp"
#include "blob.hpp"
#include "network.hpp"

using namespace std;

namespace framework {

Network::Network(void) {
}

Network::~Network(void) {
}

Network::Network(const caffe::NetParameter &net, string type) {
    map<string, string> inplace_name;
    map<string, int>    inplace_cnt;


    // set graph
    if( net.has_name() )
        _name = net.name();
    else
        _name = "unknown";


    // set graph type
    if( type.length() > 0 )
        _type = type;
    else
        _type = "unknown";


    /* Preprocessing: get a blob name list
     */
    map<string, int> blob_name_list;
    for(int i = 0; i < net.input_size(); i++)
        blob_name_list[ net.input(i) ] = 0;
    for(int i = 0; i < net.layer_size(); i++) {
        const caffe::LayerParameter& lparam = net.layer(i);
        for(int j = 0; j < lparam.bottom_size(); j++)
            blob_name_list[ lparam.bottom(j) ] = 0;
        for(int j = 0; j < lparam.top_size(); j++)
            blob_name_list[ lparam.top(j) ] = 0;
    }


    /* Input blob processing
     */
    if( net.input_size() != net.input_shape_size() )
        throw runtime_error("Network::Network; input_size != input_shape_size!");

    for(int i = 0; i < net.input_size(); i++) {
        shared_ptr<Blob> p_blob = make_shared<Blob>(net.input(i));
        _nodes.push_back( p_blob );
        _name2blobs.insert( make_pair(p_blob->get_name(), p_blob) );

        vector<int> dim;
        auto ishape = net.input_shape(i);
        for(int j = 0; j < ishape.dim_size(); j++)
            dim.push_back( ishape.dim(j) );
        p_blob->set_dim(dim);
    }


    /* Layer processing
     */
    for(int i = 0; i < net.layer_size(); i++) {
        const caffe::LayerParameter& lparam = net.layer(i);
        shared_ptr<NNLayer> p_layer = create_layer( lparam );
        _nodes.push_back( p_layer );
        _name2layers.insert( make_pair(p_layer->get_name(), p_layer) );

        cout << "processing... : " << lparam.name() << endl;

        /* Bottom Connection processing
         */
        for(int j = 0; j < lparam.bottom_size(); j++) {
            string blob_name = lparam.bottom(j);
            if( inplace_name.find(blob_name) != inplace_name.end() )
                blob_name = inplace_name[blob_name];
            shared_ptr<Blob> p_blob = _name2blobs[blob_name];
            p_layer->add_input_blob( p_blob );
            p_blob->add_consumer( p_layer );
        }

        /* Top Connection processing and Creates blob nodes
         */
        for(int j = 0; j < lparam.top_size(); j++) {
            string blob_name = lparam.top(j);

            /* check that inplance representation is used
             */
            if( _name2blobs.find(blob_name) != _name2blobs.end() ) {
                if( inplace_cnt.find(blob_name) == inplace_cnt.end() )
                    inplace_cnt[blob_name] = 0;

                string new_name;
                do {
                    new_name = blob_name + "_" + to_string( inplace_cnt[blob_name] );
                    inplace_cnt[blob_name]++;
                } while( blob_name_list.find(new_name) != blob_name_list.end() );

                inplace_name[blob_name] = new_name;
                blob_name = new_name;
            }

            shared_ptr<Blob> p_blob = make_shared<Blob>(blob_name);
            p_layer->add_output_blob( p_blob );
            p_blob->add_producer( p_layer );
            _nodes.push_back( p_blob );
            _name2blobs.insert( make_pair(p_blob->get_name(), p_blob) );
        }
    }


    /* Find entry node and exit nodes
     */
    for(uint32_t i = 0; i < _nodes.size() ; i++) {
        if( _nodes[i]->get_indegree() == 0 )
            _entry_nodes.push_back( _nodes[i] );
        if( _nodes[i]->get_outdegree() == 0 )
            _exit_nodes.push_back( _nodes[i] );
    }


    /* Output blob size calculation
     */
    _sched_layers = ScheduleLayers();
    for(auto layer : _sched_layers)
        layer->ComputeOutputSize();

#if 1   // DEBUG
    for(auto layer: _sched_layers) {
        cout << "name = " << layer->get_name();
        cout << "\ttype = " << layer->get_layer_type_str() << endl;
    }
#endif
    
    return;
}

shared_ptr<NNLayer> Network::create_layer(const caffe::LayerParameter& lparam) {
    if( ! lparam.has_type() )
        throw runtime_error("Network::create_layer; layer don't have type parameter!");

    string type = lparam.type();
    if( type.compare("Convolution") == 0 ) {
        shared_ptr<ConvLayer> layer = make_shared<ConvLayer>(lparam);
        return static_pointer_cast<NNLayer>(layer);
    }
    else if( type.compare("ReLU") == 0 ) {
        shared_ptr<ReluLayer> layer = make_shared<ReluLayer>(lparam);
        return static_pointer_cast<NNLayer>(layer);
    }
    else if( type.compare("Pooling") == 0 ) {
        shared_ptr<PoolLayer> layer = make_shared<PoolLayer>(lparam);
        return static_pointer_cast<NNLayer>(layer);
    }
    else if( type.compare("InnerProduct") == 0 ) {
        shared_ptr<FullyConnectedLayer> layer = make_shared<FullyConnectedLayer>(lparam);
        return static_pointer_cast<NNLayer>(layer);
    }
    else if( type.compare("Concat") == 0 ) {
        shared_ptr<ConcatLayer> layer = make_shared<ConcatLayer>(lparam);
        return static_pointer_cast<NNLayer>(layer);
    }
    else if( type.compare("Softmax") == 0 ) {
        shared_ptr<SoftmaxLayer> layer = make_shared<SoftmaxLayer>(lparam);
        return static_pointer_cast<NNLayer>(layer);
    }
    else if( type.compare("Input") == 0 ) {
        shared_ptr<InputLayer> layer = make_shared<InputLayer>(lparam);
        return static_pointer_cast<NNLayer>(layer);
    }
    else if( type.compare("BatchNorm") == 0 ) {
        shared_ptr<BatchNormLayer> layer = make_shared<BatchNormLayer>(lparam);
        return static_pointer_cast<NNLayer>(layer);
    }
    else if( type.compare("Scale") == 0 ) {
        shared_ptr<ScaleLayer> layer = make_shared<ScaleLayer>(lparam);
        return static_pointer_cast<NNLayer>(layer);
    }
    else if( type.compare("Dropout") == 0 ) {
        shared_ptr<DropoutLayer> layer = make_shared<DropoutLayer>(lparam);
        return static_pointer_cast<NNLayer>(layer);
    }
    else {
        cerr << "[ERROR] unsupported Layer type: " << type << endl;
        throw runtime_error("Network::create_layer; Not supported layer type!");
    }

}

/*
shared_ptr<Blob> Network::get_blob_by_name(string name) {
    shared_ptr<Node> p = _name2node[name];
    auto bp = dynamic_pointer_cast<Blob>(p);
    if( bp == nullptr )
        throw runtime_error("Network::get_blob_by_name; node is not blob.");

    return bp;
}
*/

vector<shared_ptr<NNLayer>> Network::ScheduleLayers(void) {
    map<shared_ptr<Node>, bool> vf;    // visit flag
    stack<shared_ptr<Node>> vs;        // visit stack
    vector<shared_ptr<NNLayer>> nnlayer_list;
    shared_ptr<Node> np;
    
    /* Initialize visi flags
     */
    for(uint32_t i = 0; i < _nodes.size() ; i++)
        vf.insert(make_pair(_nodes[i], false));

    /* DFS traverses
     */
    for(auto entry_nd : _entry_nodes)
        vs.push( entry_nd );

    while( ! vs.empty() ) {
        np = vs.top();
        vs.pop();

        if( vf[ np ] == false ) {
            vf[ np ] = true;

            /* Save the node pointer if the node is NNLayer
             */
            shared_ptr<NNLayer> nnlayer;
            if( (nnlayer = dynamic_pointer_cast<NNLayer>( np )) )
                nnlayer_list.push_back( nnlayer );

            for(int i = np->get_outdegree()-1; i >= 0; i--) {
                if( vf[ np->get_successor(i) ] == false ) {
                    auto succ = np->get_successor(i);
                    /* check that the node can be visited.
                     * Condition of visit: predecessors of the node
                     * should be already visited.
                     */
                    bool visit_possible = true;
                    for(auto nd : succ->get_predecessor())
                        if( vf[ nd ] == false )
                            visit_possible = false;

                    if( visit_possible )
                        vs.push( np->get_successor(i) );
                }

            }
        }
    }

    return nnlayer_list;
}



void Network::WriteNetworkToDotFile(string filename) {
    ofstream file;
    file.open( filename.c_str(), ios::out );

    file << "digraph " << _name << "{" << endl;
    file << "\trankdir = UD;" << endl;
    file << "\tnode [shape=oval]" << endl;

    vector<shared_ptr<Node>> schedList;
    schedList = BfsSchedule( _entry_nodes[0] );

    for(auto node_iter : schedList ) {
        for(auto iter : node_iter->get_successor() ) {
            if( node_iter->get_type().compare("layer") == 0 ) {
                shared_ptr<NNLayer> layer = dynamic_pointer_cast<NNLayer>(node_iter);
                file << "\t\"" << node_iter->get_name() << "_L" \
                     << "\" [shape=box,style=filled,fillcolor=\".7 .3 1.0\", label=\"" \
                     << node_iter->get_name() << layer->getLayerInfoStr() << "\"]\n";
                file << "\t\"" << node_iter->get_name() << "_L\" -> \"" \
                               << iter->get_name() << "_B\";\n";
            }
            if( node_iter->get_type().compare("blob") == 0 ) {
                shared_ptr<Blob> blob = dynamic_pointer_cast<Blob>(node_iter);
                file << "\t\"" << node_iter->get_name() << "_B" \
                     << "\" [fontsize=10, label=\"" \
                     << node_iter->get_name() << "\\n" << blob->getSizeInfoStr() << "\"]\n";
                file << "\t\"" << node_iter->get_name() << "_B\" -> \"" \
                               << iter->get_name() << "_L\";\n";
            }
        }
    }

    /* Last data node processing
     * (last data node don't have successor node)
     */
    for(auto node : _exit_nodes) {
        shared_ptr<Blob> blob = dynamic_pointer_cast<Blob>(node);
        if( blob != nullptr ) {
            file << "\t\"" << node->get_name() << "_B" \
                 << "\" [fontsize=10, label=\"" \
                 << node->get_name() << "\\n" << blob->getSizeInfoStr() << "\"]\n";
        }
    }

    file << "}" << endl;

    file.close();
}

}   // namespace framework
