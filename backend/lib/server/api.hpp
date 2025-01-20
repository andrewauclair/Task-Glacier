#ifndef MICRO_TASK_API_HPP
#define MICRO_TASK_API_HPP

#include "server.hpp"
#include "packets.hpp"

#include <vector>

class API
{
public:
	API(const Clock& clock, std::ostream& output) : m_clock(&clock), m_app(clock, output) {}

	void process_packet(const Message& message, std::vector<std::unique_ptr<Message>>& output);

private:
	const Clock* m_clock;
	MicroTask m_app;
};

#endif
