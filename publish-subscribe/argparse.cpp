#include "argparse.h"

// Arguments
string dir;
boost::asio::ip::address ip;
ushort port;
uint dims;
uint nitems;
uint worker_count;
bool verbose_flag;

// Parse arguments
bool parse_args(int argc, char* argv[])
{
	po::options_description desc("Allowed options");
	
	desc.add_options()
	("help,h", "print usage message")
	("verbose,v", "verbose mode")
	("dir,d", po::value(&dir), "server saving path")
	("ip,i", po::value<string>(), "ip for listener")
	("port,p", po::value(&port), "port for listener")
	("dims,t", po::value(&dims), "dimension of positions")
	("nitems,n", po::value(&nitems), "max number of elements")
	("workers,w", po::value(&worker_count), "worker threads");
	
	// Parse argc and argv
	po::variables_map vm;
	store(po::parse_command_line(argc, argv, desc), vm);
	
	// Help flag just print usage
	if(vm.count("help"))
	{
		cout<<desc<<endl;
		return false;
	}

	// Verbose flag
	verbose_flag = vm.count("verbose");
	
	// Port
	if(!vm.count("port"))
	{
		cout<<desc<<endl;
		return false;
	}
	port = max((ushort)1, vm["port"].as<ushort>());
	
	// Dims
	if(!vm.count("dims"))
	{
		cout<<desc<<endl;
		return false;
	}
	dims = max(1u, vm["dims"].as<uint>());

	// Nitems
	if(!vm.count("nitems"))
	{
		cout<<desc<<endl;
		return false;
	}
	nitems = max(1u, vm["nitems"].as<uint>());

	// Workers
	if(!vm.count("workers"))
	{
		cout<<desc<<endl;
		return false;
	}
	worker_count = max(1u, vm["workers"].as<uint>());

	// Dir
	if (!vm.count("dir"))
	{
		cout << desc << endl;
		return false;
	}
	dir = vm["dir"].as<string>();

	// Ip
	if (!vm.count("ip"))
	{
		cout << desc << endl;
		return false;
	}
	string ip_str = vm["ip"].as<string>();
	try 
	{
		ip = boost::asio::ip::address::from_string(ip_str);
	}
	catch (const exception& ex)
	{
		cout << "Ip Address `"<< ip_str<< "` is invalid" << endl;
		return false;
	}
	

	return true;
}

