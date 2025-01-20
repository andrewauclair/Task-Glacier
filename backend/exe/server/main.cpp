#include "packets.hpp"
#include "server.hpp"

#include <iostream>
#include <cstdlib>
#include <unordered_map>
#include <array>
#include <fstream>

#include <sockpp/tcp_acceptor.h>

inline std::uint32_t read_u32(const std::vector<std::byte>& input, std::size_t index)
{
	std::array<std::byte, 4> bytes;
	std::memcpy(bytes.data(), input.data() + index, 4);
	std::uint32_t t = {};
	std::memcpy(&t, bytes.data(), 4);
	t = std::byteswap(t);
	return t;
}

class RequestCounter
{
public:
	RequestID newID()
	{
		return nextID++;
	}

private:
	RequestID nextID = RequestID(1);
};

struct Visitor : MessageVisitor
{
	MicroTask& server;
	sockpp::tcp_socket& socket;

	Visitor(MicroTask& server, sockpp::tcp_socket& socket)
		: server(server), socket(socket)
	{
	}

	virtual void visit(const CreateTaskMessage& message) override
	{
		std::cout << message << '\n';

		const auto result = server.create_task(message.name, message.parentID);

		if (result)
		{
			SuccessResponse response(message.requestID);
			const auto output = response.pack();
			socket.write_n(output.data(), output.size());
		}
		else
		{
			FailureResponse response(message.requestID, result.error());
			const auto output = response.pack();
			socket.write_n(output.data(), output.size());
		}
	}

	void visit(const StartTaskMessage& message) override
	{
		std::cout << message << '\n';

		const auto result = server.start_task(message.taskID);

		if (result)
		{
			SuccessResponse response(message.requestID);
			const auto output = response.pack();
			socket.write_n(output.data(), output.size());
		}
		else
		{
			FailureResponse response(message.requestID, result.value());
			const auto output = response.pack();
			socket.write_n(output.data(), output.size());
		}		
	}

	void visit(const StopTaskMessage& message) override
	{
		std::cout << message << '\n';

		const auto result = server.stop_task(message.taskID);

		if (result)
		{
			SuccessResponse response(message.requestID);
			const auto output = response.pack();
			socket.write_n(output.data(), output.size());
		}
		else
		{
			FailureResponse response(message.requestID, result.value());
			const auto output = response.pack();
			socket.write_n(output.data(), output.size());
		}
	}

	void visit(const FinishTaskMessage& message) override
	{
		std::cout << message << '\n';

		const auto result = server.finish_task(message.taskID);

		if (result)
		{
			SuccessResponse response(message.requestID);
			const auto output = response.pack();
			socket.write_n(output.data(), output.size());
		}
		else
		{
			FailureResponse response(message.requestID, result.value());
			const auto output = response.pack();
			socket.write_n(output.data(), output.size());
		}
	}

	virtual void visit(const SuccessResponse& message) override
	{
		std::cout << message << '\n';
	}

	virtual void visit(const FailureResponse& message) override
	{
		std::cout << message << '\n';
	}
	
	virtual void visit(const BasicMessage& message) override
	{
		std::cout << message << '\n';
	}
	
	virtual void visit(const TaskInfoMessage& message) override
	{
		std::cout << message << '\n';
	}
};

/*
* task-glacier 127.0.0.1 5000 /var/lib/task-glacier
*/
int main(int argc, char** argv)
{
	if (argc < 4)
	{
		std::cerr << "task-glacier <ip address> <port> <persistence directory>\n";
		return -1;
	}

	sockpp::initialize();

	const std::string ip_address = argv[1];

	const int port = std::atoi(argv[2]);

	std::cout << ip_address << " " << port << '\n';

	auto acceptor = sockpp::tcp_acceptor(sockpp::inet_address(ip_address, port));

	auto connection = acceptor.accept();

	auto socket = std::make_unique<sockpp::tcp_socket>(std::move(connection));

	std::cout << "connected\n";

	Clock clock;
	std::ofstream output(argv[3]);
	MicroTask server(clock, output);
	Visitor visitor(server, *socket);

	while (socket->is_open())
	{
		std::vector<std::byte> input(4);
		socket->read_n(input.data(), 4);

		const auto length = read_u32(input, 0);

		input.resize(length);
		socket->read_n(input.data() + 4, length - 4);

		const auto result = parse_packet(input);

		if (result.packet)
		{
			result.packet->visit(visitor);
		}
	}
}
