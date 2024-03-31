#ifndef MICRO_TASK_API_HPP
#define MICRO_TASK_API_HPP

#include "lib.hpp"
#include "packets.hpp"

#include <vector>

class API
{
public:
	void process_packet(const Message& message, std::vector<MessageTypes>& output);

private:
	MicroTask m_app;
};

#endif
