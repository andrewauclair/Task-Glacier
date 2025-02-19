#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include <libassert/assert.hpp>

#include "api.hpp"
#include "server.hpp"
#include "packets.hpp"
#include "utils.h"

#include <vector>
#include <source_location>

TEST_CASE("Configuring Bugzilla Information", "[bugzilla][api]")
{
	TestHelper helper;

	// send bugzilla packet
	auto configure = BugzillaInfoMessage("bugzilla", "asfesdFEASfslj");
	configure.username = "test";
	configure.rootTaskID = TaskID(5);
	configure.groupTasksBy = "severity";

	configure.labelToField["Priority"] = "priority";
	configure.labelToField["Status"] = "status";

	helper.api.process_packet(configure, helper.output);

	SECTION("Information is Set in Memory")
	{
		auto request = BasicMessage(PacketType::REQUEST_CONFIGURATION);

		helper.api.process_packet(request, helper.output);

		helper.required_messages({ &configure });
	}

	SECTION("Information is Persisted")
	{
		// TODO this is all temporary. we need something setup to use, this will have to do. persistence will just be a log of steps to rebuild our data
		CHECK(helper.fileOutput.str() == "bugzilla: bugzilla asfesdFEASfslj\ntest\n5\nseverity\n2\nPriority\npriority\nStatus\nstatus\n");
	}
}

TEST_CASE("Initial Bugzilla Refresh Pulls All Open Bugs", "[bugzilla][api]")
{
	TestHelper helper;

	// send bugzilla packet
	auto configure = BugzillaInfoMessage("bugzilla", "asfesdFEASfslj");
	configure.username = "test";
	configure.rootTaskID = TaskID(5);
	configure.groupTasksBy = "severity";

	configure.labelToField["Priority"] = "priority";
	configure.labelToField["Status"] = "status";

	helper.api.process_packet(configure, helper.output);

	// send bugzilla refresh packet
	const auto refresh = RequestMessage(PacketType::BUGZILLA_REFRESH, helper.next_request_id());

	helper.expect_success(refresh);

	CHECK(helper.curl.request == "bugzilla/rest/bug?assigned_to=test&resolution=---&api_key=asfesdFEASfslj");
	
	// verify various operations
}

TEST_CASE("Subsequent Bugzilla Refresh Requests Updates Since Last Refresh", "[bugzilla][api]")
{
	TestHelper helper;

	// send bugzilla packet
	auto configure = BugzillaInfoMessage("bugzilla", "asfesdFEASfslj");
	configure.username = "test";
	configure.rootTaskID = TaskID(5);
	configure.groupTasksBy = "severity";

	configure.labelToField["Priority"] = "priority";
	configure.labelToField["Status"] = "status";

	helper.api.process_packet(configure, helper.output);

	// send bugzilla refresh packet
	const auto refresh = RequestMessage(PacketType::BUGZILLA_REFRESH, helper.next_request_id());

	helper.expect_success(refresh);
	helper.expect_success(refresh);

	// verify curl request has previous refresh date
	CHECK(helper.curl.request == "bugzilla/rest/bug?assigned_to=test&resolution=---&last_update_time=&api_key=asfesdFEASfslj");

	// verify various operations
}

TEST_CASE("Reloading Bugzilla Information From File", "[bugzilla][api]")
{

}
