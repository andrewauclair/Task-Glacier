#pragma once

#include "packets/message.hpp"


struct PacketSender
{
	virtual ~PacketSender() = default;

	virtual void send(const Message& message) = 0;
};
