#ifndef ARGPARSE_H
#define ARGPARSE_H

#include <iostream>
#include <string>

#include <boost/program_options.hpp>
#include <boost/asio.hpp>
namespace po = boost::program_options;
using namespace std;

typedef unsigned int uint;
typedef unsigned short ushort;

extern string dir;
extern boost::asio::ip::address ip;
extern ushort port;
extern uint dims;
extern uint nitems;
extern uint worker_count;
extern bool verbose_flag;

bool parse_args(int argc, char* argv[]);


#endif // ARGPARSE_H
