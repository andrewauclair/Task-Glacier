#include "cli.hpp"

#include <iostream>

/*
* micro_task_cli.exe 127.0.0.1 5000
*/
int main(int argc, char** argv)
{
	if (argc < 3) return -1;

	sockpp::initialize();

	const std::string ip_address = argv[1];

	const int port = std::atoi(argv[2]);

	std::cout << ip_address << " " << port << '\n';

	MicroTaskCLI cli;
	cli.connect(ip_address, port);

	bool running = true;
	RequestID nextRequestID = RequestID(1);

	while (running)
	{
		std::string command;
		std::getline(std::cin, command);

		if (command == "exit")
		{
			running = false;
		}
		else if (command.starts_with("add task"))
		{
			std::string task = command.substr(command.find_first_of('"') + 1, command.find_last_of('"'));

			const RequestID requestID = nextRequestID++;
			CreateTaskMessage message(NO_PARENT, requestID, task);

			cli.send_message(message);
		}
		else if (command.starts_with("start task"))
		{
			const std::int32_t taskID = std::stoll(command.substr(command.find("start task")));

			const RequestID requestID = nextRequestID++;
			//BasicMessage message(PacketType::START_TASK);
		}
		else if (command.starts_with("stop task"))
		{

		}
		else if (command.starts_with("finish task"))
		{

		}
	}
}
