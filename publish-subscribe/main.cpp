#include <iostream>
#include <boost/filesystem.hpp>
#include "argparse.h"
#include "socket.h"

bool prepare_dir()
{
	try
	{
		// Create mkdir
		boost::filesystem::create_directory(dir);

		// Remove all files
		boost::filesystem::path path_to_remove(dir);

		for (boost::filesystem::directory_iterator end_dir_it, it(path_to_remove); it != end_dir_it; it++)
			boost::filesystem::remove_all(it->path());

		return true;
	}
	catch (const exception& e)
	{
		cout << e.what() << endl;
		return false;
	}

}


int main(int argc, char** argv)
{
	if (!parse_args(argc, argv))
	{
		return 0;
	}

	if (verbose_flag)
		cout << "Verbose flag set" << endl;
	else
	{
		// disable output
		std::cout.setstate(std::ios_base::failbit);
	}
	if (!prepare_dir())
	{
		exit(0);
	}

	Service service;
	service.start_workers();

	Server server(ip, port, &service);
	server.run();

	return 0;
}