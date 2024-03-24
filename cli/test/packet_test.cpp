#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include <libassert/assert.hpp>

#include "lib.hpp"
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
	std::vector<std::byte> bytes;

	PacketVerifier& required_length(std::size_t length)
	{
		REQUIRE(bytes.size() == length);

		return *this;
	}

	template<typename T>
	PacketVerifier& verify_type(std::size_t start, T expected)
	{
		return verify_bytes(start, sizeof(T), convert_to_bytes(std::byteswap(expected)));
	}

	PacketVerifier& verify_bytes(std::size_t start, std::size_t length, const std::vector<std::byte>& expected_bytes)
	{
		CHECK_THAT(std::span(bytes).subspan(start, length), Catch::Matchers::RangeEquals(expected_bytes));
		return *this;
	}

	PacketVerifier& verify_string(std::size_t start, const std::string& string)
	{
		const std::uint16_t size = string.length();

		CHECK_THAT(std::span(bytes).subspan(start, 2), Catch::Matchers::RangeEquals(::bytes((size & 0xFF00) >> 8, size & 0xFF)));

		auto string_bytes = std::span(bytes).subspan(start + 2, string.length());
		std::string str;
		for (auto byte : string_bytes)
		{
			str.push_back(static_cast<char>(byte));
		}
		CHECK_THAT(str, Catch::Matchers::Equals(string, Catch::CaseSensitive::Yes));
		return *this;
	}
};

TEST_CASE("pack the create list message", "[list][message][pack]")
{
	PacketBuilder builder;

	CreateListMessage create_list{ GroupID(5), RequestID(10), "testing"};

	const auto packed = create_list.pack();

	REQUIRE(packed.size() == 25);

	// packet length
	CHECK_THAT(std::span(packed).subspan(0, 4), Catch::Matchers::RangeEquals(bytes(0, 0, 0, 25)));

	// packet ID
	CHECK_THAT(std::span(packed).subspan(4, 4), Catch::Matchers::RangeEquals(bytes(0, 0, 0, 2)));

	// request ID
	CHECK_THAT(std::span(packed).subspan(8, 4), Catch::Matchers::RangeEquals(bytes(0, 0, 0, 10)));

	// group ID
	CHECK_THAT(std::span(packed).subspan(12, 4), Catch::Matchers::RangeEquals(bytes(0, 0, 0, 5)));

	// name length
	CHECK_THAT(std::span(packed).subspan(16, 2), Catch::Matchers::RangeEquals(bytes(0, 7)));

	// name
	CHECK_THAT(std::span(packed).subspan(18, 7), Catch::Matchers::RangeEquals(bytes('t', 'e', 's', 't', 'i', 'n', 'g')));

	auto verifier = PacketVerifier{ create_list.pack() };

	verifier.required_length(25)
		.verify_type<std::uint32_t>(0, 25)
		.verify_type<std::uint32_t>(4, 2)
		.verify_type<std::uint32_t>(8, 10)
		.verify_type<std::uint32_t>(12, 5)
		.verify_string(16, "testing");
}

class CreateListMessageMatcher : public Catch::Matchers::MatcherBase<CreateListMessage>
{
public:
	CreateListMessageMatcher(const CreateListMessage& expected) : m_expected(expected) {}

	bool match(const CreateListMessage& actual) const override
	{
		CHECK(m_expected.groupID == actual.groupID);
		CHECK(m_expected.requestID == actual.requestID);
		CHECK(m_expected.name == actual.name);
		return true;
	}

	std::string describe() const override
	{
		return std::format("{{ groupID: {}, requestID: {}, name: {} }}", m_expected.groupID, m_expected.requestID._val, m_expected.name);
	}

private:
	const CreateListMessage& m_expected;
};

TEST_CASE("parse a create list packet", "[list][message][unpack]")
{
	MicroTask app;

	CreateListMessage create_list{ 5, RequestID(10), "testing" };
	
	// handle the packet
	const auto result = parse_packet(create_list.pack());

	REQUIRE(result.packet.has_value());

	auto packet = std::get<CreateListMessage>(result.packet.value());

	CHECK(packet.groupID == 5);
	CHECK(packet.requestID == RequestID(10));
	CHECK(packet.name == "testing");

	CHECK(result.bytes_read == 25);
}

TEST_CASE("pack the create group message", "[group][message][pack]")
{
	PacketBuilder builder;

	CreateGroupMessage create_group{ 5, 10, "test_group" };

	const auto packed = create_group.pack();

	REQUIRE(packed.size() == 28);

	// packet length
	CHECK_THAT(std::span(packed).subspan(0, 4), Catch::Matchers::RangeEquals(bytes(0, 0, 0, 28)));

	// packet ID
	CHECK_THAT(std::span(packed).subspan(4, 4), Catch::Matchers::RangeEquals(bytes(0, 0, 0, 3)));

	// request ID
	CHECK_THAT(std::span(packed).subspan(8, 4), Catch::Matchers::RangeEquals(bytes(0, 0, 0, 10)));

	// group ID
	CHECK_THAT(std::span(packed).subspan(12, 4), Catch::Matchers::RangeEquals(bytes(0, 0, 0, 5)));

	// name length
	CHECK_THAT(std::span(packed).subspan(16, 2), Catch::Matchers::RangeEquals(bytes(0, 10)));

	// name
	CHECK_THAT(std::span(packed).subspan(18, 10), Catch::Matchers::RangeEquals(bytes('t', 'e', 's', 't', '_', 'g', 'r', 'o', 'u', 'p')));
}

TEST_CASE("parse create group packet", "[group][message][unpack]")
{
	MicroTask app;

	auto create_group = CreateGroupMessage(5, 10, "test_group");

	const auto result = parse_packet(create_group.pack());

	REQUIRE(result.packet.has_value());

	auto packet = std::get<CreateGroupMessage>(result.packet.value());

	CHECK(packet.groupID == 5);
	CHECK(packet.requestID == 10);
	CHECK(packet.name == "test_group");

	CHECK(result.bytes_read == 28);
}