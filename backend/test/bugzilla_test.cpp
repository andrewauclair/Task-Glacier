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

		auto complete = BasicMessage(PacketType::REQUEST_CONFIGURATION_COMPLETE);

		helper.required_messages({ &configure, &complete });
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
	// initial tasks are created
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
	helper.clock.time += std::chrono::hours(2);
	helper.expect_success(refresh);

	// verify curl request has previous refresh date
	CHECK(helper.curl.request == "bugzilla/rest/bug?assigned_to=test&resolution=---&api_key=asfesdFEASfslj&last_change_time=2025-01-20T05:48:59Z");

	// verify various operations
	// new tasks are added
	// tasks are reparented if their grouping changes
	// tasks are finished if the bug is resolved
	// labels are added and removed
}

// TODO this is all temporary. we need something setup to use, this will have to do. persistence will just be a log of steps to rebuild our data
TEST_CASE("Bugzilla Persistence", "[bugzilla][api]")
{
	TestHelper helper;

	SECTION("Save")
	{
		auto configure = BugzillaInfoMessage("bugzilla", "asfesdFEASfslj");
		configure.username = "test";
		configure.rootTaskID = TaskID(5);
		configure.groupTasksBy = "severity";

		configure.labelToField["Priority"] = "priority";
		configure.labelToField["Status"] = "status";

		helper.api.process_packet(configure, helper.output);

		const auto refresh = RequestMessage(PacketType::BUGZILLA_REFRESH, helper.next_request_id());

		helper.expect_success(refresh);
		helper.clock.time += std::chrono::hours(2);
		helper.expect_success(refresh);

		CHECK(helper.fileOutput.str() == "bugzilla-config bugzilla asfesdFEASfslj\ntest\n5\nseverity\n2\nPriority\npriority\nStatus\nstatus\nbugzilla-refresh 1737344039870\nbugzilla-refresh 1737353039870\n");
	}

	SECTION("Load")
	{
		helper.fileOutput << "bugzilla-config bugzilla asfesdFEASfslj\ntest\n5\nseverity\n2\nPriority\npriority\nStatus\nstatus\nbugzilla-refresh 1737344039870\nbugzilla-refresh 1737353039870\n";
		helper.fileInput = std::istringstream(helper.fileOutput.str());
		helper.fileOutput.clear();

		helper.api = API(helper.clock, helper.curl, helper.fileInput, helper.fileOutput);

		// now that we're setup, request the configuration and check the output
		helper.api.process_packet(BasicMessage{ PacketType::REQUEST_CONFIGURATION }, helper.output);

		CHECK(helper.output.size() == 2);

		auto configure = BugzillaInfoMessage("bugzilla", "asfesdFEASfslj");
		configure.username = "test";
		configure.rootTaskID = TaskID(5);
		configure.groupTasksBy = "severity";

		configure.labelToField["Priority"] = "priority";
		configure.labelToField["Status"] = "status";

		auto complete = BasicMessage(PacketType::REQUEST_CONFIGURATION_COMPLETE);

		helper.required_messages({ &configure, &complete });
	}
}