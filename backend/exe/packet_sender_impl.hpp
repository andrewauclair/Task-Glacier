#pragma once

#include "packet_sender.hpp"

#include <sockpp/tcp_acceptor.h>

#include <iostream>
#include <fstream>

extern std::ofstream logfile;

struct PacketSenderImpl : PacketSender
{
	sockpp::tcp_socket* socket;

	PacketSenderImpl(sockpp::tcp_socket* socket) : socket(socket) {}

	void send(std::unique_ptr<Message> message) override
	{
		auto time = std::chrono::system_clock::now();

		std::cout << std::format("[{:%m/%d/%y %H:%M:%S}]", time) << " [TX] " << *message << '\n';

		logfile << std::format("[{:%m/%d/%y %H:%M:%S}]", time) << " [TX] " << *message << '\n';

		const auto output = message->pack();
		socket->write_n(output.data(), output.size());
	}
};
