#ifndef MICRO_TASK_API_HPP
#define MICRO_TASK_API_HPP

#include "server.hpp"
#include "packets.hpp"

#include <vector>

class API
{
public:
	void process_packet(const Message& message, std::vector<std::unique_ptr<Message>>& output);

private:
	MicroTask m_app;
};

#endif
