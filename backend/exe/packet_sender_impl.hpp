#pragma once

#include "packet_sender.hpp"

#include <sockpp/tcp_acceptor.h>

#include <iostream>

struct PacketSenderImpl : PacketSender
{
	sockpp::tcp_socket* socket;

	PacketSenderImpl(sockpp::tcp_socket* socket) : socket(socket) {}

	void send(const Message& message) override
	{
		std::cout << "[TX] " << message << '\n';

		const auto output = message.pack();
		socket->write_n(output.data(), output.size());
	}
};
