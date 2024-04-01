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

class CreateListMessageMatcher : public Catch::Matchers::MatcherBase<CreateListMessage>
{
public:
	CreateListMessageMatcher(const CreateListMessage& expected) : m_expected(expected) {}

	mutable std::ostringstream issues;
	bool match(const CreateListMessage& actual) const override
	{
		if (m_expected.groupID != actual.groupID) {
			issues << "groupID\n";
		}
		return m_expected.groupID == actual.groupID &&
			m_expected.requestID == actual.requestID &&
			m_expected.name == actual.name;
	}

	std::string explain() const
	{
		return issues.str();
	}

	std::string describe() const override
	{
		return std::format("{{ groupID: {}, requestID: {}, name: {} }}", m_expected.groupID, m_expected.requestID._val, m_expected.name);
	}

private:
	const CreateListMessage& m_expected;
};

//template<typename ArgT, typename MatcherT>
//class CustomMatchExpr : public Catch::ITransientExpression {
//	ArgT&& m_arg;
//	MatcherT const& m_matcher;
//public:
//	CustomMatchExpr(ArgT&& arg, MatcherT const& matcher)
//		: Catch::ITransientExpression{ true, matcher.match(arg) }, // not forwarding arg here on purpose
//		m_arg(CATCH_FORWARD(arg)),
//		m_matcher(matcher)
//	{}
//
//	void streamReconstructedExpression(std::ostream& os) const override {
//		os << Catch::Detail::stringify(m_arg)
//			<< ' '
//			<< m_matcher.toString() << '\n';
//
//		os << m_matcher.explain();
//	}
//};
//template<typename ArgT, typename MatcherT>
//auto makeCustomMatchExpr(ArgT&& arg, MatcherT const& matcher) -> CustomMatchExpr<ArgT, MatcherT> {
//	return CustomMatchExpr<ArgT, MatcherT>(CATCH_FORWARD(arg), matcher);
//}
/////////////////////////////////////////////////////////////////////////////////
//#define CUSTOM_INTERNAL_CHECK_THAT( macroName, matcher, resultDisposition, arg ) \
//    do { \
//        Catch::AssertionHandler catchAssertionHandler( macroName##_catch_sr, CATCH_INTERNAL_LINEINFO, CATCH_INTERNAL_STRINGIFY(arg) ", " CATCH_INTERNAL_STRINGIFY(matcher), resultDisposition ); \
//        INTERNAL_CATCH_TRY { \
//            catchAssertionHandler.handleExpr( makeCustomMatchExpr( arg, matcher ) ); \
//        } INTERNAL_CATCH_CATCH( catchAssertionHandler ) \
//        INTERNAL_CATCH_REACT( catchAssertionHandler ) \
//    } while( false )
//
//
//#define CUSTOM_CHECK_THAT( arg, matcher ) CUSTOM_INTERNAL_CHECK_THAT( "CHECK_THAT", matcher, Catch::ResultDisposition::ContinueOnFailure, arg )

//TEST_CASE("compare", "[list]")
//{
//	CreateListMessage a{ 5, RequestID(10), "testing" };
//	CreateListMessage b{ 6, RequestID(10), "testing" };
//
//	CUSTOM_CHECK_THAT(b, CreateListMessageMatcher(a));
//}

TEST_CASE("pack the create task message", "[task][message][pack]")
{
	const auto create_task = CreateTaskMessage(ListID(5), RequestID(10), "this is a test");

	auto verifier = PacketVerifier(create_task.pack(), 32);

	verifier
		.verify_value<std::uint32_t>(32, "packet length")
		.verify_value<std::uint32_t>(1, "packet ID")
		.verify_value<std::uint32_t>(10, "request ID")
		.verify_value<std::uint32_t>(5, "list ID")
		.verify_string("this is a test", "task name");
}

TEST_CASE("parse a create task packet", "[task][message][unpack]")
{
	MicroTask app;

	const auto create_task = CreateTaskMessage(ListID(5), RequestID(10), "this is a test");

	const auto result = parse_packet(create_task.pack());

	REQUIRE(result.packet.has_value());

	const auto packet = dynamic_cast<CreateTaskMessage&>(*result.packet.value().get());

	CHECK(packet == create_task);
	CHECK(result.bytes_read == 32);
}

TEST_CASE("pack the create list message", "[list][message][pack]")
{
	auto create_list = CreateListMessage(GroupID(5), RequestID(10), "testing");

	auto verifier = PacketVerifier(create_list.pack(), 25);

	verifier
		.verify_value<std::uint32_t>(25, "packet length")
		.verify_value<std::uint32_t>(2, "packet ID")
		.verify_value<std::uint32_t>(10, "request ID")
		.verify_value<std::uint32_t>(5, "group ID")
		.verify_string("testing", "list name");
}

TEST_CASE("parse a create list packet", "[list][message][unpack]")
{
	MicroTask app;

	const auto create_list = CreateListMessage(GroupID(5), RequestID(10), "testing");
	
	// handle the packet
	const auto result = parse_packet(create_list.pack());

	REQUIRE(result.packet.has_value());

	const auto packet = dynamic_cast<CreateListMessage&>(*result.packet.value().get());

	CHECK(packet == create_list);
	CHECK(result.bytes_read == 25);
}

TEST_CASE("pack the create group message", "[group][message][pack]")
{
	PacketBuilder builder;

	const auto create_group = CreateGroupMessage(GroupID(5), RequestID(10), "test_group");

	auto verifier = PacketVerifier(create_group.pack(), 28);

	verifier
		.verify_value<std::uint32_t>(28, "packet length")
		.verify_value<std::uint32_t>(3, "packet ID")
		.verify_value<std::uint32_t>(10, "request ID")
		.verify_value<std::uint32_t>(5, "group ID")
		.verify_string("test_group", "group name");
}

TEST_CASE("parse create group packet", "[group][message][unpack]")
{
	MicroTask app;

	const auto create_group = CreateGroupMessage(GroupID(5), RequestID(10), "test_group");

	const auto result = parse_packet(create_group.pack());

	REQUIRE(result.packet.has_value());

	const auto packet = dynamic_cast<CreateGroupMessage&>(*result.packet.value().get());
	
	CHECK(packet == create_group);
	CHECK(result.bytes_read == 28);
}

TEST_CASE("pack the success response packet", "[message][pack]")
{
	PacketBuilder builder;

	const auto response = SuccessResponse(RequestID(15));

	auto verifier = PacketVerifier(response.pack(), 12);

	verifier
		.verify_value<std::uint32_t>(12, "packet length")
		.verify_value<std::uint32_t>(10, "packet ID")
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
		.verify_value<std::uint32_t>(11, "packet ID")
		.verify_value<std::uint32_t>(10, "request ID")
		.verify_string("this is a failure message", "error message");
}

// don't need to care about parsing FailureResponse atm
