#include "server.hpp"
#include "api.hpp"
#include "curl.hpp"
#include "packet_sender_impl.hpp"

#include <iostream>
#include <cstdlib>
#include <unordered_map>
#include <array>
#include <fstream>

#include <sockpp/tcp_acceptor.h>

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>

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

struct curlpp_ : cURL
{
	// TODO probably return a JSON object
	std::string execute_request(const std::string& url) override
	{
		try {
			curlpp::Cleanup cleaner;

			std::ostringstream ss;
			ss << curlpp::options::Url(url);

			curlpp::Easy request;

			request.setOpt(new curlpp::options::Url(url));
			request.setOpt(new curlpp::options::Verbose(true));

			request.perform();
			return ss.str();
		}
		catch (const curlpp::LogicError& e) {
			std::cout << e.what() << std::endl;
		}
		catch (const curlpp::RuntimeError& e) {
			std::cout << e.what() << std::endl;
		}
		return "";
	}
};

/*
* task-glacier 127.0.0.1 5000 /var/lib/task-glacier.db3
*/
int main(int argc, char** argv)
{
	if (argc < 4)
	{
		std::cerr << "task-glacier <ip address> <port> <database>\n";
		return -1;
	}

	sockpp::initialize();

	curlpp_ curl;
	DatabaseImpl db(argv[3]);

	const std::string ip_address = argv[1];

	const int port = std::atoi(argv[2]);

	std::cout << ip_address << ' ' << port << ' ' << argv[3] << '\n';

	auto acceptor = sockpp::tcp_acceptor(sockpp::inet_address(ip_address, port));

	Clock clock;

	API api(clock, curl, db);

	// ctrl-c app to kill it
	while (true)
	{
		auto connection = acceptor.accept();

		std::cout << "Connected\n";

		auto socket = std::make_unique<sockpp::tcp_socket>(std::move(connection));

		auto sender = PacketSenderImpl{ socket.get() };

		while (socket->is_open())
		{
			std::vector<std::byte> input(4);
			if (socket->read_n(input.data(), 4) == -1)
			{
				std::cout << "Error with socket\n";
				std::cout << socket->last_error() << '\n';

				break;
			}

			const auto length = read_u32(input, 0);

			input.resize(length);
			if (socket->read_n(input.data() + 4, length - 4) == -1)
			{
				std::cout << "Error with socket\n";
				std::cout << socket->last_error() << '\n';

				break;
			}

			const auto result = parse_packet(input);

			if (result.packet)
			{
				std::cout << "[RX] " << *result.packet << '\n';

				std::vector<std::unique_ptr<Message>> toSend;
				api.process_packet(*result.packet, toSend);

				for (auto&& message : toSend)
				{
					//std::cout << "[TX] " << *message << '\n';

					/*const auto output = message->pack();
					socket->write_n(output.data(), output.size());*/
					sender.send(*message);
				}
			}
		}

		std::cout << "Disconnected\n";
	}
}
