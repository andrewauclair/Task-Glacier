#ifndef MICRO_TASK_API_HPP
#define MICRO_TASK_API_HPP

#include "lib.hpp"
#include "packets.hpp"

#include <vector>

class API
{
public:
	API(std::vector<MessageTypes>& output);

	void process_packet(const MessageTypes& message);

private:
	std::vector<MessageTypes>& m_output;

	MicroTask m_app;
};

#endif
