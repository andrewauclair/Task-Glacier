#ifndef MICRO_TASK_CLI_HPP
#define MICRO_TASK_CLI_HPP

#include <sockpp/tcp_connector.h>

#include <string>

class MicroTaskCLI
{
public:
	~MicroTaskCLI();

	bool connect(const std::string& ip, int port);
private:
	sockpp::tcp_connector connector;
};

#endif
