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

	helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 1"));

	// send bugzilla packet
	auto configure = BugzillaInfoMessage("bugzilla", "asfesdFEASfslj");
	configure.username = "test";
	configure.rootTaskID = TaskID(1);
	configure.groupTasksBy = "severity";

	configure.labelToField["Priority"] = "priority";
	configure.labelToField["Status"] = "status";

	helper.api.process_packet(configure, helper.output);

	// send bugzilla refresh packet
	const auto refresh = RequestMessage(PacketType::BUGZILLA_REFRESH, helper.next_request_id());

	helper.curl.result = "{ \"bugs\": [ { \"id\": 50, \"summary\": \"bug 1\", \"status\": Assigned, \"priority\": Normal, \"severity\": Minor },"
									   "{ \"id\": 55, \"summary\": \"bug 2\", \"status\": Changes Made, \"priority\": Normal, \"severity\": Major },"
									   "{ \"id\": 60, \"summary\": \"bug 3\", \"status\": Changes Made, \"priority\": Low, \"severity\": Critical },"
									   "{ \"id\": 65, \"summary\": \"bug 4\", \"status\": Reviewed, \"priority\": High, \"severity\": Blocker },"
									   "{ \"id\": 70, \"summary\": \"bug 5\", \"status\": Confirmed, \"priority\": Highest, \"severity\": Nitpick } ] }";

	helper.expect_success(refresh);

	CHECK(helper.curl.request == "bugzilla/rest/bug?assigned_to=test&resolution=---&api_key=asfesdFEASfslj");

	auto taskInfo2 = TaskInfoMessage(TaskID(2), NO_PARENT, "50 - bug 1");
	auto taskInfo3 = TaskInfoMessage(TaskID(3), NO_PARENT, "50 - bug 2");
	auto taskInfo4 = TaskInfoMessage(TaskID(4), NO_PARENT, "50 - bug 3");
	auto taskInfo5 = TaskInfoMessage(TaskID(5), NO_PARENT, "50 - bug 4");
	auto taskInfo6 = TaskInfoMessage(TaskID(6), NO_PARENT, "50 - bug 5");

	taskInfo2.createTime = std::chrono::milliseconds(1737344039870);
	taskInfo3.createTime = std::chrono::milliseconds(1737344039870);
	taskInfo4.createTime = std::chrono::milliseconds(1737344039870);
	taskInfo5.createTime = std::chrono::milliseconds(1737344039870);
	taskInfo6.createTime = std::chrono::milliseconds(1737344039870);
	taskInfo2.newTask = true;
	taskInfo3.newTask = true;
	taskInfo4.newTask = true;
	taskInfo5.newTask = true;
	taskInfo6.newTask = true;

	helper.required_messages({ &taskInfo2, &taskInfo3, &taskInfo4, &taskInfo5, &taskInfo6 });

	auto complete = BasicMessage(PacketType::REQUEST_CONFIGURATION_COMPLETE);

	helper.required_messages({ &configure, &complete });

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