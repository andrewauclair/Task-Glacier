#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include <libassert/assert.hpp>

#include "server.hpp"
#include "packets.hpp"

std::vector<std::byte> bytes(auto... a)
{
	return std::vector<std::byte>{ static_cast<std::byte>(a)... };
}

template<typename T>
std::vector<std::byte> convert_to_bytes(T value)
{
	std::vector<std::byte> bytes(sizeof(T));
	std::memcpy(bytes.data(), &value, sizeof(T));

	return bytes;
}

struct PacketVerifier
{
private:
	std::vector<std::byte> m_bytes;
	std::size_t m_current_pos = 0;

public:
	PacketVerifier(std::vector<std::byte> bytes, std::size_t required_length)
		: m_bytes(std::move(bytes))
	{
		REQUIRE(m_bytes.size() == required_length);
	}

	template<typename T>
	PacketVerifier& verify_value(T expected, std::string_view field_name)
	{
		INFO("field: " << field_name << ", expected value: " << expected);

		CHECK_THAT(std::span(m_bytes).subspan(m_current_pos, sizeof(T)), Catch::Matchers::RangeEquals(convert_to_bytes(std::byteswap(expected))));
		m_current_pos += sizeof(T);
		return *this;
	}

	PacketVerifier& verify_string(const std::string& string, std::string_view field_name)
	{
		INFO("field: " << field_name << ", expected value: " << string);

		const std::uint16_t size = string.length();
		const auto size_type_length = sizeof(size);
		
		if (m_current_pos + size >= m_bytes.size())
		{
			FAIL("not enough bytes");
		}

		CHECK_THAT(std::span(m_bytes).subspan(m_current_pos, size_type_length), Catch::Matchers::RangeEquals(bytes((size & 0xFF00) >> 8, size & 0xFF)));
		
		auto string_bytes = std::span(m_bytes).subspan(m_current_pos + size_type_length, string.length());
		
		std::string str;
		
		for (auto byte : string_bytes)
		{
			str.push_back(static_cast<char>(byte));
		}
		
		m_current_pos += size_type_length + size;

		CHECK_THAT(str, Catch::Matchers::Equals(string, Catch::CaseSensitive::Yes));

		return *this;
	}
};

TEST_CASE("pack the create task message", "[task][message][pack]")
{
	const auto create_task = CreateTaskMessage(TaskID(5), RequestID(10), "this is a test");

	auto verifier = PacketVerifier(create_task.pack(), 32);

	verifier
		.verify_value<std::uint32_t>(32, "packet length")
		.verify_value<std::uint32_t>(3, "packet ID")
		.verify_value<std::uint32_t>(10, "request ID")
		.verify_value<std::uint32_t>(5, "parent ID")
		.verify_string("this is a test", "task name");
}

TEST_CASE("parse a create task packet", "[task][message][unpack]")
{
	MicroTask app;

	const auto create_task = CreateTaskMessage(NO_PARENT, RequestID(10), "this is a test");

	const auto result = parse_packet(create_task.pack());

	REQUIRE(result.packet.has_value());

	const auto packet = dynamic_cast<CreateTaskMessage&>(*result.packet.value().get());

	CHECK(packet == create_task);
	CHECK(result.bytes_read == 32);
}

TEST_CASE("pack the success response packet", "[message][pack]")
{
	PacketBuilder builder;

	const auto response = SuccessResponse(RequestID(15));

	auto verifier = PacketVerifier(response.pack(), 12);

	verifier
		.verify_value<std::uint32_t>(12, "packet length")
		.verify_value<std::uint32_t>(8, "packet ID")
		.verify_value<std::uint32_t>(15, "request ID");
}

// don't need to care about parsing SuccessResponse atm

TEST_CASE("pack the failure response packet", "[message][pack]")
{
	PacketBuilder builder;

	const auto response = FailureResponse(RequestID(10), "this is a failure message");

	auto verifier = PacketVerifier(response.pack(), 39);

	verifier
		.verify_value<std::uint32_t>(39, "packet length")
		.verify_value<std::uint32_t>(9, "packet ID")
		.verify_value<std::uint32_t>(10, "request ID")
		.verify_string("this is a failure message", "error message");
}

// don't need to care about parsing FailureResponse atm

TEST_CASE("pack the empty packet", "[message][pack]")
{
	PacketBuilder builder;

	const auto message = EmptyMessage(PacketType::REQUEST_CONFIGURATION);

	auto verifier = PacketVerifier(message.pack(), 8);

	verifier
		.verify_value<std::uint32_t>(8, "packet length")
		.verify_value<std::uint32_t>(10, "packet ID");
}

TEST_CASE("unpack the empty packet", "[message][unpack]")
{
	MicroTask app;

	const auto message = EmptyMessage(PacketType::REQUEST_CONFIGURATION_COMPLETE);

	const auto result = parse_packet(message.pack());

	REQUIRE(result.packet.has_value());

	const auto packet = dynamic_cast<EmptyMessage&>(*result.packet.value().get());

	CHECK(packet == message);
	CHECK(result.bytes_read == 8);
}