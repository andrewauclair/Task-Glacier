#include "cli.hpp"

MicroTaskCLI::~MicroTaskCLI()
{
	shutdown = true;
	read_thread.join();
}

bool MicroTaskCLI::connect(const std::string& ip, int port)
{
	connector = sockpp::tcp_connector(sockpp::inet_address(ip, port));

	if (connector.is_connected())
	{
		start_read_thread();
	}
	return connector.is_connected();
}

void MicroTaskCLI::send_message(const Message& message)
{
	if (!connector.is_connected())
	{
		throw std::runtime_error("No connection to server. Failed to send message.");
	}

	const std::vector<std::byte> bytes = message.pack();

	if (connector.write_n(bytes.data(), bytes.size()) == -1)
	{
		throw std::runtime_error("Failed to write data to socket.");
	}
}

void MicroTaskCLI::start_read_thread()
{
	//read_thread = std::thread(read);
}

void MicroTaskCLI::read()
{
	while (!shutdown && connector.is_connected())
	{
	}
}