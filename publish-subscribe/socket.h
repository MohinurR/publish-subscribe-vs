#ifndef SOCKET_H
#define SOCKET_H

#define BUFFER_SIZE 1024

#include <iostream>
#include <vector>
//#include <Windows.h>
#include <stdio.h>
using namespace std;

typedef unsigned int uint;
typedef unsigned short ushort;

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

class Session;
class Service;

#include "service.h"

class Session: public std::enable_shared_from_this<Session>
{
	public:
		Session(boost::asio::io_service& io_service, Service* service);
		void start();
		void write(string msg);
		void handle_write(const boost::system::error_code& error);
		tcp::socket& get_socket();	
		
		void chat_on(std::shared_ptr<Session> chat);
		void chat_off();

	private:
		tcp::socket socket_;
		void handle_read(std::shared_ptr<Session>& s, const boost::system::error_code& error, const size_t length);
		char buffer[BUFFER_SIZE];
		Service* service_;

		bool chat_mode;
		CRITICAL_SECTION chat_sem;
		std::shared_ptr<Session> chat;
		deque<string> write_msgs;

};


class Server
{
	public:
		Server(boost::asio::ip::address address, ushort port, Service* service);

		void start_accept();
		void handle_accept(std::shared_ptr<Session> session, const boost::system::error_code& error);
		void run();

	private:
		boost::asio::io_service io_service_;
		tcp::acceptor acceptor_;
		Service* service_;
};

#endif // SOCKET_H
