#include "server.hpp"
#include "api.hpp"
#include "curl.hpp"
#include "packet_sender_impl.hpp"
#include "packets/packet_parser.hpp"

#include <iostream>
#include <cstdlib>
#include <unordered_map>
#include <array>
#include <fstream>

#include <sockpp/tcp_acceptor.h>

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>

#ifdef _MSC_VER
#include <Windows.h>
#endif

std::ofstream logfile;

void log_message(const std::string& message)
{
	auto time = std::chrono::system_clock::now();

	std::cout << std::format("[{:%m/%d/%y %H:%M:%S}] ", time) << message << '\n';
	logfile << std::format("[{:%m/%d/%y %H:%M:%S}] ", time) << message << '\n';
}

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
	std::optional<std::string> execute_request(const std::string& url) override
	{
		try {
			curlpp::Cleanup cleaner;

			std::ostringstream ss;
			ss << curlpp::options::Url(url);

			curlpp::Easy request;

			std::ostringstream os;
			curlpp::options::WriteStream ws(&os);
			request.setOpt(ws);
			request.setOpt(new curlpp::options::Url(url));
			request.setOpt(new curlpp::options::Verbose(true));

			request.perform();
			return std::optional<std::string>(os.str());
		}
		catch (const curlpp::LogicError& e)
		{
			log_message("cURL error");
			log_message(e.what());
		}
		catch (const curlpp::RuntimeError& e)
		{
			log_message("cURL error");
			log_message(e.what());
		}
		return std::nullopt;
	}
};

/*
* task-glacier 127.0.0.1 5000 /var/lib/task-glacier.db3
*/
int main(int argc, char** argv)
{
	if (argc < 5)
	{
		std::cerr << "task-glacier <ip address> <port> <database> <logfile> <hidden>\n";
		return -1;
	}

	logfile = std::ofstream(argv[4]);

	sockpp::initialize();

	curlpp_ curl;
	

	const std::string ip_address = argv[1];

	const int port = std::atoi(argv[2]);

	std::stringstream arg_output;
	arg_output << ip_address << ' ' << port << ' ' << argv[3] << ' ' << argv[4] << '\n';

	log_message(arg_output.str());

	bool hidden = argc > 5 && std::string(argv[5]) == "true";

	if (hidden)
	{
#ifdef _MSC_VER
		FreeConsole();
#endif
	}

	auto acceptor = sockpp::tcp_acceptor(sockpp::inet_address(ip_address, port));

	Clock clock;

	// ctrl-c app to kill it
	while (true)
	{
		auto connection = acceptor.accept();

		std::cout << "Connected\n";

		logfile << "Connected\n";

		auto socket = std::make_unique<sockpp::tcp_socket>(std::move(connection));

		auto sender = PacketSenderImpl{ socket.get() };
		DatabaseImpl db(argv[3], sender);

		API api(clock, curl, db, sender);

		while (socket->is_open())
		{
			auto now = clock.now();

			std::vector<std::byte> input(4);
			if (socket->read_n(input.data(), 4) == -1)
			{
				log_message("Error with socket");
				log_message(std::to_string(socket->last_error()));

				break;
			}

			const auto length = read_u32(input, 0);

			input.resize(length);
			if (socket->read_n(input.data() + 4, length - 4) == -1)
			{
				log_message("Error with socket");
				log_message(std::to_string(socket->last_error()));

				break;
			}

			const auto result = parse_packet(input);

			if (result.packet)
			{
				std::stringstream ss;
				ss << "[RX] " << *result.packet;

				log_message(ss.str());

				api.process_packet(*result.packet);
			}
		}

		std::cout << "Disconnected\n";

		logfile << "Disconnected\n";
	}
}
