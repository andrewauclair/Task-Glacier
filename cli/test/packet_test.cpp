#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include "lib.hpp"
#include "packets.hpp"

std::vector<std::byte> bytes(auto... a)
{
	return std::vector<std::byte>{ static_cast<std::byte>(a)... };
}

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

template<typename ArgT, typename MatcherT>
class CustomMatchExpr : public Catch::ITransientExpression {
	ArgT&& m_arg;
	MatcherT const& m_matcher;
public:
	CustomMatchExpr(ArgT&& arg, MatcherT const& matcher)
		: Catch::ITransientExpression{ true, matcher.match(arg) }, // not forwarding arg here on purpose
		m_arg(CATCH_FORWARD(arg)),
		m_matcher(matcher)
	{}

	void streamReconstructedExpression(std::ostream& os) const override {
		os << Catch::Detail::stringify(m_arg)
			<< ' '
			<< m_matcher.toString() << '\n';

		os << m_matcher.explain();
	}
};
template<typename ArgT, typename MatcherT>
auto makeCustomMatchExpr(ArgT&& arg, MatcherT const& matcher) -> CustomMatchExpr<ArgT, MatcherT> {
	return CustomMatchExpr<ArgT, MatcherT>(CATCH_FORWARD(arg), matcher);
}
///////////////////////////////////////////////////////////////////////////////
#define CUSTOM_INTERNAL_CHECK_THAT( macroName, matcher, resultDisposition, arg ) \
    do { \
        Catch::AssertionHandler catchAssertionHandler( macroName##_catch_sr, CATCH_INTERNAL_LINEINFO, CATCH_INTERNAL_STRINGIFY(arg) ", " CATCH_INTERNAL_STRINGIFY(matcher), resultDisposition ); \
        INTERNAL_CATCH_TRY { \
            catchAssertionHandler.handleExpr( makeCustomMatchExpr( arg, matcher ) ); \
        } INTERNAL_CATCH_CATCH( catchAssertionHandler ) \
        INTERNAL_CATCH_REACT( catchAssertionHandler ) \
    } while( false )


#define CUSTOM_CHECK_THAT( arg, matcher ) CUSTOM_INTERNAL_CHECK_THAT( "CHECK_THAT", matcher, Catch::ResultDisposition::ContinueOnFailure, arg )

TEST_CASE("compare", "[list]")
{
	CreateListMessage a{ 5, RequestID(10), "testing" };
	CreateListMessage b{ 6, RequestID(10), "testing" };

	CUSTOM_CHECK_THAT(b, CreateListMessageMatcher(a));
}

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
}

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