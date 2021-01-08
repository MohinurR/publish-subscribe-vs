#include "service.h"


Service::Service()
{
	//Init queue
	InitializeCriticalSection(&queue_sem);

	// Init publishers
	InitializeCriticalSection(&publishers_sem);

	// Init subscribers
	InitializeCriticalSection(&subscribers_sem);
	
}

Service::~Service()
{
	working = false;
}

void Service::start_workers()
{
	// Init workers
	working = true;
	workers = new thread[worker_count];

	for(size_t i=0; i<worker_count; i++)
	{
		workers[i] = thread(&Service::worker_function, this);
		workers[i].detach();
	}
	
}

void Service::add_message(string msg, shared_ptr<Session> sess)
{
	EnterCriticalSection(&queue_sem);
	msg_queue.push(make_pair(msg, sess));
	LeaveCriticalSection(&queue_sem);
}

void Service::worker_function()
{
	string cmd;
	string address;
	uint pos;
	set<uint> items;
	
	publisher pubs;
	subscriber subs;
	string err;

	while(working)
	{
		EnterCriticalSection(&queue_sem);
		if(!msg_queue.empty())
		{
			//print_all();
			
			auto entry = msg_queue.front();
			msg_queue.pop();
			LeaveCriticalSection(&queue_sem);
			
			// [Optional] Message trimming
			boost::trim(entry.first);

			if(parse_msg(entry.first, cmd, address, pos, items) && constraint_check(pos, items, err))
			{

				// Message details
				cout<<"Command: "<<cmd<<endl;
				cout<<"Address: "<<address<<endl;
				cout<<"Position: "<<pos<<endl;
				cout<<"Items: ";

				for(uint item: items)
					cout<<item<<" ";
				cout<<endl;
				// [END] Message details
				if(cmd=="subscribe") 
				{
					cout << "Subscribe\n";
					// New subscriber with publisher found
					if(new_subscriber(pos, items, entry.second, pubs, err))
					{	
						cout << "Found publisher: " << pubs.address << endl;
						// Enable chat mode
						entry.second->write("!ready");
						pubs.session->write("!ready");

						entry.second->chat_on(pubs.session);
						pubs.session->chat_on(entry.second);
					}


					// Error
					else if(err!="")
					{
						entry.second->write("!error "+err);
					}	
				}

				else if (cmd=="publish")
				{
					cout << "Publish\n";
					// New publisher with subscriber found
					if(new_publisher(address, pos, items, entry.second, subs, err))
					{	
						cout << "Found subscriber: " << endl;

						// Enable chat mode
						subs.session->write("!ready");
						entry.second->write("!ready");

						entry.second->chat_on(subs.session);
						subs.session->chat_on(entry.second);

					}

					// Error
					else if (err!="")
					{
						entry.second->write("!error "+err);
					}
				}
				
				// No error ok message
				if(err=="")
					entry.second->write("!ok");
			}
			else
			{
				// Invalid details
				cout<<"Invalid: "<<entry.first<<endl;
				entry.second->write("!error Invalid message "+err);
			}


		}
		else
		{
			LeaveCriticalSection(&queue_sem);
		}
		
	}
}

void Service::print_all()
{
	cout<<"Publishers:\n";
	
	EnterCriticalSection(&publishers_sem);

	for(auto it=publishers.begin(); it!=publishers.end(); it++)
	{
		cout << it->first << " : " << it->second.address << endl;
		
	}
	LeaveCriticalSection(&publishers_sem);

	EnterCriticalSection(&subscribers_sem);
	
	for(auto it=subscribers.begin(); it!=subscribers.end(); it++)
	{
		cout << it->first << " : " << it->second.session << endl;
	}

	LeaveCriticalSection(&subscribers_sem);

}


// Handler for new publisher
bool Service::new_publisher(string address, uint pos, set<uint> items, shared_ptr<Session> sess, subscriber& ret, string& err)
{
	// No error
	err = "";

	// Find corresponding subscriber
	EnterCriticalSection(&subscribers_sem);
	auto it = subscribers.find(pos);

	// Found
	if (it != subscribers.end())
	{
		bool found = (it->second.items == items);

		// Found exit function
		if (found)
		{
			ret = it->second;
			subscribers.erase(it);

			remove(pos, "s");
			LeaveCriticalSection(&subscribers_sem);
			return true;
		}
	}

	LeaveCriticalSection(&subscribers_sem);


	// Not found
	EnterCriticalSection(&publishers_sem);

	publisher pubs;
	pubs.session = sess;
	pubs.address = address;
	pubs.items = items;

	auto publisher_it = publishers.find(pos);

	if (publisher_it == publishers.end())
	{
		publishers[pos] = pubs;

		write(pos, "p", pubs.items);
	}
	else
	{
		//if(publisher_it->second.session=sess)
		//{
		//	publisher_it->second.items.insert(items.begin(), items.end());
		//}
		//else
		//{
		//	err = "Publisher already exists"; 
		//}
		err = "Publisher already exists";
	}

	LeaveCriticalSection(&publishers_sem);

	return false;

}

// Handler for new subscriber
bool Service::new_subscriber(uint pos, set<uint> items, shared_ptr<Session> sess, publisher& ret, string& err)
{

	// No error
	err = "";

	// Find corresponding publisher
	EnterCriticalSection(&publishers_sem);
	auto it = publishers.find(pos);

	// Found
	if (it != publishers.end())
	{
		bool found = (it->second.items == items);

		// Found exit function
		if (found)
		{
			ret = it->second;
			publishers.erase(it);

			remove(pos, "p");
			// Unlock semaphore
			LeaveCriticalSection(&publishers_sem);
			return true;
		}

	}

	// Unlock semaphore
	LeaveCriticalSection(&publishers_sem);


	// Not found
	EnterCriticalSection(&subscribers_sem);

	subscriber subs;
	subs.session = sess;
	subs.items = items;

	auto subscriber_it = subscribers.find(pos);

	if (subscriber_it == subscribers.end())
	{
		// New subs to publishers
		subscribers[pos] = subs;
		write(pos, "s", subs.items);
	}
	else
	{
		//if(subscriber_it->second.session==sess)
		//{
		//	subscriber_it->second.items.insert(items.begin(), items.end());
		//}

		//else
		//{
		//	// Error subsriber already exists
		//	err = "Subscriber already exists";
		//}
		err = "Subscriber already exists";

	}
	LeaveCriticalSection(&subscribers_sem);

	return false;
}



bool parse_msg(string msg, string&cmd, string& address, uint& pos, set<uint>& items)
{
	boost::regex subscribe_regex("subscribe <([0-9]+)> <([0-9,]*[0-9])>");
	boost::regex publish_regex("publish <([^>]+)> <([0-9]+)> <([0-9,]*[0-9])>");
	boost::smatch matches;
	
	// Clear items
	items.clear();

	if(boost::regex_match(msg, matches, subscribe_regex))
	{
		cmd = "subscribe";
		address = "";
		pos = stoi(matches[1]);
		
		// Parse items
		vector<string> str_items;
		boost::split(str_items, matches[2], boost::is_any_of(","));
		//items.reserve(str_items.size());
		for(string str : str_items)
		{
			boost::trim(str);
			items.insert( atoi(str.c_str()) );
		}

		return true;
	}
	else if (boost::regex_match(msg, matches, publish_regex))
	{
		cmd = "publish";
		address = string(matches[1]);
		pos = stoi(matches[2]);
		// Parse items
		vector<string> str_items;
		boost::split(str_items, matches[3], boost::is_any_of(","));
		//items.reserve(str_items.size());
		for(string str : str_items)
		{
			boost::trim(str);
			items.insert( atoi(str.c_str()) );
		}
		return true;
	}
	
	return false;
}

bool operator==(publisher const& a, publisher const& b)
{
	return a.session==b.session && a.address==b.address && a.items==b.items;
}

bool operator==(subscriber const& a, subscriber const& b)
{
	return a.session==b.session && a.items==b.items;
}

bool constraint_check(const uint& pos, const set<uint>& items, string& err)
{
	if(pos<1 || pos>dims)
	{
		err = "Position is not in constraint";
		return false;
	}
	
	for(uint item: items)
	{
		if(item<1 || item>nitems)
		{
			err = "Item is not in constraint";
			return false;
		}
	}

	return true;
}


bool write(uint pos, const string type, const set<uint>& items)
{
	boost::filesystem::path p(dir);
	p /= type + "_" + to_string(pos) + ".bin";

	ofstream file(p.string(), ifstream::binary | ifstream::trunc);

	char* buffer = new char[nitems];
	for (auto item : items)
		buffer[item - 1] = (char)1;

	file.write(buffer, nitems);
	file.close();
	delete[] buffer;

	return true;
}

bool remove(uint pos, string type)
{
	boost::filesystem::path p(dir);
	p /= type + "_" + to_string(pos) + ".bin";

	boost::filesystem::remove(p);
	return true;
}
