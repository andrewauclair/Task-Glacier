#pragma once

#include "packets/message.hpp"


struct PacketSender
{
	virtual ~PacketSender() = default;

	virtual void send(std::unique_ptr<Message> message) = 0;
};
