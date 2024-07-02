#include "cli.hpp"

bool MicroTaskCLI::connect(const std::string& ip, int port)
{
	connector = sockpp::tcp_connector(sockpp::inet_address(ip, port));

	return connector.is_connected();
}
