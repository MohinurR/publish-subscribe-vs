#ifndef SERVICE_H
#define SERVICE_H

#include <iostream>
#include <string>
#include <vector>
//#include <Windows.h>
#include <queue>
#include <deque>
#include <unordered_map>
#include <thread>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

class Service;

#include "socket.h"
#include "argparse.h"

using namespace std;

struct publisher
{
	shared_ptr<Session> session;
	string address;
	set <uint> items;
};

bool operator==(publisher const& a, publisher const& b);

struct subscriber
{
	shared_ptr<Session> session;
	set <uint> items;
};

bool operator==(subscriber const& a, subscriber const& b);


class Service
{
	private:
		queue< pair<string, shared_ptr<Session> > >msg_queue;
		CRITICAL_SECTION queue_sem;
		
		unordered_map< uint, publisher > publishers;
		CRITICAL_SECTION publishers_sem;
		
		unordered_map< uint, subscriber > subscribers;
		CRITICAL_SECTION subscribers_sem;
		
		thread* workers;
		bool working;

	public:

		Service();
		void start_workers();
		void worker_function();
		void add_message(string msg, shared_ptr<Session> sess);

		~Service();
	
	private:
		bool new_publisher(string address, uint pos, set<uint> items, shared_ptr<Session> sess, subscriber& ret, string& err);
		bool new_subscriber(uint pos, set<uint> items, shared_ptr<Session> sess, publisher& ret, string& err);
		void print_all();
};

bool parse_msg(string msg, string& cmd, string& address, uint& pos, set<uint>& items);

bool constraint_check(const uint& pos, const set<uint>& items, string& err);

bool parse_msg(string msg, string& cmd, string& address, uint& pos, set<uint>& items);

bool constraint_check(const uint& pos, const set<uint>& items, string& err);

// Write to files
bool write(uint pos, const string type, const set<uint>& items);

// Remove
bool remove(uint pos, const string type);

#endif // SERVICE_H
