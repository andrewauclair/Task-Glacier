#ifndef MICRO_TASK_PACKETS_HPP
#define MICRO_TASK_PACKETS_HPP

#include <memory>

#include <string>
#include <vector>
#include <array>

#include <sockpp/tcp_socket.h>

#include <nlohmann/json.hpp>

// packet structure
// ----------------------------------------------
// | length (2) | command (1) | sub-command (1) |
// ----------------------------------------------
// 
// version request, response
// add task
// start task
// stop task
// finish task

// add list

// add group
enum class commands
{
	VERSION_REQUEST = 1,
	VERSION_RESPONSE = 2,

	ADD = 3,
	START = 4,
	STOP = 5,
	FINISH = 6,
};

enum class add_sub_commands
{
	TASK = 1,
	LIST = 2,
	GROUP = 3,
	PROJECT = 4,
	FEATURE = 5,
	MILESTONE = 6,
};

class command_packet
{
public:
	command_packet(commands command) 
		: m_command(command)
	{
	}

	commands command() const { return m_command; }

private:
	commands m_command;
};

class version_request : public command_packet
{
public:
	version_request() : command_packet(commands::VERSION_REQUEST) {}
};

class version_response : public command_packet
{
public:
	version_response(std::string version) 
		: command_packet(commands::VERSION_RESPONSE),
		version(std::move(version))
	{
	}

private:
	std::string version;
};

struct add_packet : public command_packet
{
	add_sub_commands sub_command;
};

struct add_task_packet : public add_packet
{

};

namespace mt
{
	inline std::uint16_t read_u16(const std::vector<std::byte>& input, std::size_t index)
	{
		std::array<std::byte, 2> bytes;
		std::memcpy(bytes.data(), input.data() + index, 2);
		std::uint16_t t = {};
		std::memcpy(&t, bytes.data(), 2);
		return t;
	}

	inline std::optional<nlohmann::json> read_next_packet(sockpp::tcp_socket& socket)
	{
		static std::vector<std::byte> received;

		std::array<std::byte, 512> buffer;

		if (received.size() > 0)
		{
			const std::uint16_t packet_length = ntohs(read_u16(received, 0));

			if (received.size() >= packet_length)
			{
				std::vector<char> chars;

				std::transform(received.begin() + 2, received.begin() + packet_length,
					std::back_inserter(chars),
					[](std::byte byte) { return static_cast<char>(byte); });

				received.erase(received.begin(), received.begin() + packet_length);

				const auto json = std::string(chars.begin(), chars.end());
				
				return nlohmann::json::parse(json);
			}
		}

		while (true)
		{
			const auto read = socket.read(buffer.data(), buffer.size());
			std::cout << "read: " << read << '\n';

			if (read == -1) return {};
			
			std::copy_n(buffer.begin(), read, std::back_inserter(received));
			
			const std::uint16_t packet_length = ntohs(read_u16(received, 0));
			
			if (received.size() >= packet_length)
			{
				break;
			}
		}

		const std::uint16_t packet_length = ntohs(read_u16(received, 0));

		std::vector<char> chars;
		 
		std::transform(received.begin() + 2, received.begin() + packet_length, 
			std::back_inserter(chars),
			[](std::byte byte) { return static_cast<char>(byte); });

		received.erase(received.begin(), received.begin() + packet_length);

		const auto json = std::string(chars.begin(), chars.end());
		
		return nlohmann::json::parse(json);
	}

	inline void send_packet(sockpp::tcp_socket& socket, const nlohmann::json& json)
	{
		std::string str = json.dump();

		const std::int16_t size = htons(static_cast<std::int16_t>(str.length() + 2));

		socket.write_n(&size, 2);
		socket.write_n(str.data(), str.length());
	}
}

#endif
