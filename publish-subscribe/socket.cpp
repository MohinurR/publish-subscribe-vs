#include "socket.h"

// Session
Session::Session(boost::asio::io_service& io_service, Service* service):socket_(io_service), service_(service)
{
	chat_mode = false;
	chat = nullptr;

	InitializeCriticalSection(&chat_sem);
}

// Start function
void Session::start()
{
	socket_.async_read_some( 
			boost::asio::buffer(buffer, BUFFER_SIZE),
			boost::bind(&Session::handle_read, this, shared_from_this(),
						boost::asio::placeholders::error, 
						boost::asio::placeholders::bytes_transferred));
}

// Read handler
void Session::handle_read(std::shared_ptr<Session>& s, const boost::system::error_code& error, const size_t length)
{
	if(!error)
	{
		socket_.async_read_some( 
			boost::asio::buffer(buffer, BUFFER_SIZE),
			boost::bind(&Session::handle_read, this, shared_from_this(),
						boost::asio::placeholders::error, 
						boost::asio::placeholders::bytes_transferred));
		buffer[length]='\0';
		//printf("Received message: %s", buffer);
		
		// Chat check
		EnterCriticalSection(&chat_sem);
		
		string msg = string(buffer);
		boost::trim(msg);
		if(chat_mode)
		{
			if(msg=="!stop")
			{
				chat->chat_off();
				
				LeaveCriticalSection(&chat_sem);
				chat_off();
				
			}
			else
				chat->write(msg);

		}
		else
			service_->add_message(msg, shared_from_this());
		
		LeaveCriticalSection(&chat_sem);
		// [END] Chat check
	}
	else
	{
		EnterCriticalSection(&chat_sem);
		if(chat_mode)
		{
			chat->write("!error " + error.message());
			chat->chat_off();
		}
		
		LeaveCriticalSection(&chat_sem);

		chat_off();
	}
}

// Write message
void Session::write(string msg)
{
	bool write_in_progress = !write_msgs.empty();
	write_msgs.push_back(msg);
	if (!write_in_progress)
	{
		boost::asio::async_write(socket_,
			boost::asio::buffer(write_msgs.front().data(),
				write_msgs.front().length()),
			boost::bind(&Session::handle_write, shared_from_this(),
				boost::asio::placeholders::error));
	}
}

void Session::handle_write(const boost::system::error_code& error)
{
	if (!error)
	{
		write_msgs.pop_front();
		if (!write_msgs.empty())
		{
			boost::asio::async_write(socket_,
				boost::asio::buffer(write_msgs.front().data(),
					write_msgs.front().length()),
				boost::bind(&Session::handle_write, shared_from_this(),
					boost::asio::placeholders::error));
		}
	}
}

// Get socket
tcp::socket& Session::get_socket()
{
	return socket_;
}


// Chat mode

void Session::chat_on(std::shared_ptr<Session> chat)
{
	EnterCriticalSection(&chat_sem);
	if (chat != nullptr)
	{
		this->chat = chat;
		chat_mode = true;
	}
	LeaveCriticalSection(&chat_sem);
}

void Session::chat_off()
{
	EnterCriticalSection(&chat_sem);

	chat_mode = false;
	chat = nullptr;

	LeaveCriticalSection(&chat_sem);
}

// [END] Chat mode


// Server
Server::Server(boost::asio::ip::address address, ushort port, Service* service): acceptor_(io_service_, tcp::endpoint(address, port)), service_(service)
{
	start_accept();
}

// Start 
void Server::start_accept()
{
	std::shared_ptr<Session> session = std::make_shared<Session>(io_service_, service_);
	acceptor_.async_accept(session->get_socket(),
			boost::bind(&Server::handle_accept, this, session,
				boost::asio::placeholders::error));
}

// Accept handler
void Server::handle_accept(std::shared_ptr<Session> session, const boost::system::error_code& error)
{
	if(!error)
		session->start();

	start_accept();
}

void Server::run()
{
	this->io_service_.run();
}
