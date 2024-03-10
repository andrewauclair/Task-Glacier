#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include "lib.hpp"
#include "packets.hpp"

class PacketBuilderOld
{
	std::vector<std::byte> m_bytes;

public:
	std::span<const std::byte> bytes() const { return m_bytes; }

	template<typename T>
	void add_value(T value)
	{
		T swapped = std::byteswap(value);
		auto* f = reinterpret_cast<unsigned char*>(&swapped);

		for (int i = 0; i < sizeof(T); i++, f++)
		{
			m_bytes.push_back(static_cast<std::byte>(*f));
		}
	}

	void add_string(std::string_view str)
	{
		std::int16_t size = str.size();

		add_value(size);

		for (auto ch : str)
		{
			m_bytes.push_back(static_cast<std::byte>(ch));
		}
	}
};

std::vector<std::byte> bytes(auto... a)
{
	return std::vector<std::byte>{ static_cast<std::byte>(a)... };
}

TEST_CASE("pack the create list message", "[list][message][pack]")
{
	PacketBuilder builder;

	CreateListMessage create_list{ GroupID(5), 10, "testing"};

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
}

TEST_CASE("parse a create list packet", "[list][message][unpack]")
{
	MicroTask app;

	CreateListMessage create_list{ 5, 10, "testing" };
	
	// handle the packet
	const auto result = parse_packet(create_list.pack());

	REQUIRE(result.packet.has_value());

	auto packet = std::get<CreateListMessage>(result.packet.value());

	CHECK(packet.groupID == 5);
	CHECK(packet.requestID == 10);
	CHECK(packet.name == "testing");

	CHECK(result.bytes_read == 25);
}

TEST_CASE("pack the create group message", "[group][message][pack]")
{
	PacketBuilder builder;

	CreateGroupMessage create_group{ 5, 10, "test_group" };

	const auto packed = create_group.pack();

	REQUIRE(packed.size() == 28);

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