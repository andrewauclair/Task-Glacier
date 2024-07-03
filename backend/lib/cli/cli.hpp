#ifndef MICRO_TASK_CLI_HPP
#define MICRO_TASK_CLI_HPP

#include "packets.hpp"

#include <sockpp/tcp_connector.h>

#include <string>
#include <thread>

class MicroTaskCLI
{
public:
	~MicroTaskCLI();

	bool connect(const std::string& ip, int port);

	void send_message(const Message& message);

private:
	void start_read_thread();
	void read();

private:
	sockpp::tcp_connector connector;
	std::thread read_thread;
	std::atomic_bool shutdown;
};

#endif
