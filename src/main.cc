#include <unistd.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <fcntl.h>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include "log.h"
#include "caffe.pb.h"
#include "graph.hpp"
#include "network.hpp"

using namespace std;
using namespace caffe;
using namespace framework;

using google::protobuf::io::FileInputStream;
using google::protobuf::io::FileOutputStream;
using google::protobuf::io::ZeroCopyInputStream;
using google::protobuf::io::CodedInputStream;
using google::protobuf::io::ZeroCopyOutputStream;
using google::protobuf::io::CodedOutputStream;
using google::protobuf::Message;

bool ReadProtoFromTextFile(const char *filename, Message *proto) {
    int fd = open(filename, O_RDONLY);
	if( fd == -1 ) {
		cerr << "File not found: " << filename << endl;
		return false;
	}

	FileInputStream *input = new FileInputStream( fd );
	bool success = google::protobuf::TextFormat::Parse(input, proto);
	delete input;
	close(fd);
	return success;
}

int main(int argc, char **argv) {
	char option;
	const char *optstring = "g:o:";

    open_log_file("log.txt");

	if( argc < 3 ) {
		cerr << "Not enough inputs." << endl;
		cerr << "nnc -g [graph file] -o [output file]" << endl;
		return -1;
	}

	string graphFileName = "graphNet.prototxt";
	string outputFileName = "output.dot";

    while( -1 != (option = getopt(argc, argv, optstring))) {
		switch(option) {
			case 'g':	graphFileName = optarg;
						break;
			case 'o':	outputFileName = optarg;
						break;

		}
	}

	caffe::NetParameter net_param;
	if( ! ReadProtoFromTextFile( graphFileName.c_str(), &net_param ) )
		return 0;

	Network network( net_param, "NN" );
	network.WriteNetworkToDotFile( outputFileName );

    close_log_file();

	return 0;
}
