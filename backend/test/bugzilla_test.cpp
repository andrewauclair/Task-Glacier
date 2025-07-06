#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include <libassert/assert.hpp>

#include "api.hpp"
#include "server.hpp"
#include "packets.hpp"
#include "utils.h"

#include <vector>
#include <source_location>





/*

persist: bugzilla info, bugzilla refresh times, mappings of all bugzilla task IDs/bug IDs

locked in after creation: name and root task

keep a mapping of all the bugzilla task IDs to bug IDs

all other sub-tasks of the root task will be set to finish before starting a refresh

pull all bug data from server

find all possible values for all group by options from the bugs

for each layer of group by, find or create the necessary task

move bugs to their proper parent, possibly one layer at a time



*/


/*
* NEW TESTS
* 
* configuring the bugzilla info
* saving/loading bugzilla info
* saving/loading bugzilla refresh time
* saving/loading bug to task ID mapping
* 
* initial refresh to add tasks
* refresh to add tasks
* refresh with resolved bug
* refresh with bug that was reassigned (not sure how to handle this)
* refresh finished grouping tasks
* refresh adds new grouping tasks
* refresh after changing group by
* refresh with 3+ layers of grouping (a bit ridiculous)
*/













TEST_CASE("Configuring Bugzilla Information", "[bugzilla][api]")
{
	TestHelper helper;

	// send bugzilla packet
	auto configure = BugzillaInfoMessage("bugzilla", "0.0.0.0", "asfesdFEASfslj");
	configure.username = "test";
	configure.rootTaskID = TaskID(5);
	configure.groupTasksBy.push_back("product");
	configure.groupTasksBy.push_back("severity");

	configure.labelToField["Priority"] = "priority";
	configure.labelToField["Status"] = "status";

	helper.curl.requestResponse.emplace_back("{ \"fields\": [] }");
	helper.curl.requestResponse.emplace_back("{ \"bugs\": [] }");

	helper.api.process_packet(configure, helper.output);

	SECTION("Information is Set in Memory")
	{
		auto request = BasicMessage(PacketType::REQUEST_CONFIGURATION);

		helper.api.process_packet(request, helper.output);

		auto timeCategories = TimeEntryDataPacket({});
		auto complete = BasicMessage(PacketType::REQUEST_CONFIGURATION_COMPLETE);

		helper.required_messages({ &configure, &timeCategories, &complete });
	}
}

TEST_CASE("Configuring Multiple Bugzilla Instances", "[bugzilla][api]")
{
	TestHelper helper;

	// send bugzilla packet
	auto configure = BugzillaInfoMessage("bugzilla", "0.0.0.0", "asfesdFEASfslj");
	configure.username = "test";
	configure.rootTaskID = TaskID(5);
	configure.groupTasksBy.push_back("product");
	configure.groupTasksBy.push_back("severity");

	configure.labelToField["Priority"] = "priority";
	configure.labelToField["Status"] = "status";

	helper.curl.requestResponse.emplace_back("{ \"fields\": [] }");

	helper.api.process_packet(configure, helper.output);

	auto configure2 = configure;
	configure2.name = "bugzilla2";

	helper.curl.current = 0;

	helper.api.process_packet(configure2, helper.output);

	SECTION("Information is Set in Memory")
	{
		auto request = BasicMessage(PacketType::REQUEST_CONFIGURATION);

		helper.api.process_packet(request, helper.output);

		auto timeCategories = TimeEntryDataPacket({});
		auto complete = BasicMessage(PacketType::REQUEST_CONFIGURATION_COMPLETE);

		helper.required_messages({ &configure, &configure2, &timeCategories, &complete });
	}
}

// TODO this is all temporary. we need something setup to use, this will have to do. persistence will just be a log of steps to rebuild our data
TEST_CASE("Bugzilla Persistence", "[bugzilla][api]")
{
	TestHelper helper;

	SECTION("Save")
	{
		auto create1 = CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 1");
		helper.expect_success(create1);

		auto configure = BugzillaInfoMessage("bugzilla", "0.0.0.0", "asfesdFEASfslj");
		configure.username = "test";
		configure.rootTaskID = TaskID(1);
		configure.groupTasksBy.push_back("product");
		configure.groupTasksBy.push_back("severity");

		configure.labelToField["Priority"] = "priority";
		configure.labelToField["Status"] = "status";

		helper.curl.requestResponse.emplace_back("{ \"fields\": [] }");

		helper.api.process_packet(configure, helper.output);

		const auto refresh = RequestMessage(PacketType::BUGZILLA_REFRESH, helper.next_request_id());

		helper.curl.requestResponse.emplace_back("{ \"bugs\": [] }");

		helper.expect_success(refresh);
		helper.curl.current = 1;
		helper.clock.time += std::chrono::hours(2);
		helper.expect_success(refresh);

		CHECK(helper.fileOutput.str() == "create 1 0 1737344939870 (6 test 1)\ntask-time-codes 1 0 0 \nbugzilla-config bugzilla 0.0.0.0 asfesdFEASfslj\ntest\n1\n2\nproduct\nseverity\n2\nPriority\npriority\nStatus\nstatus\nbugzilla-refresh bugzilla 1737345839870\nbugzilla-refresh bugzilla 1737353939870\n");
	}

	SECTION("Load")
	{
		helper.fileOutput << "bugzilla-config bugzilla 0.0.0.0 asfesdFEASfslj\ntest\n5\n2\nproduct\nseverity\n2\nPriority\npriority\nStatus\nstatus\nbugzilla-refresh bugzilla 1737344039870\nbugzilla-refresh bugzilla 1737352139870\n";
		helper.fileInput = std::istringstream(helper.fileOutput.str());
		helper.fileOutput.clear();

		helper.api = API(helper.clock, helper.curl, helper.fileInput, helper.fileOutput);

		// now that we're setup, request the configuration and check the output
		helper.api.process_packet(BasicMessage{ PacketType::REQUEST_CONFIGURATION }, helper.output);

		CHECK(helper.output.size() == 3);

		auto configure = BugzillaInfoMessage("bugzilla", "0.0.0.0", "asfesdFEASfslj");
		configure.username = "test";
		configure.rootTaskID = TaskID(5);
		configure.groupTasksBy.push_back("product");
		configure.groupTasksBy.push_back("severity");

		configure.labelToField["Priority"] = "priority";
		configure.labelToField["Status"] = "status";

		auto timeCategories = TimeEntryDataPacket({});
		auto complete = BasicMessage(PacketType::REQUEST_CONFIGURATION_COMPLETE);

		helper.required_messages({ &configure, &timeCategories, &complete });
	}
}

TEST_CASE("Bugzilla Refresh", "[bugzilla][api]")
{
	using namespace std::chrono_literals;

	TestHelper helper;

	helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "Bugzilla"));

	// send bugzilla packet
	auto configure = BugzillaInfoMessage("bugzilla", "0.0.0.0", "asfesdFEASfslj");
	configure.username = "test";
	configure.rootTaskID = TaskID(1);
	configure.groupTasksBy.push_back("priority");
	configure.groupTasksBy.push_back("severity");

	configure.labelToField["Priority"] = "priority";
	configure.labelToField["Status"] = "status";

	// task glacier will first request values for all the group tasks by fields
	helper.curl.requestResponse.emplace_back(R"(
		{
		  "fields": [
			{
			  "display_name": "Priority",
			  "name": "priority",
			  "type": 2,
			  "is_mandatory": false,
			  "value_field": null,
			  "values": [
				{
				  "sortkey": 100,
				  "sort_key": 100,
				  "visibility_values": [],
				  "name": "P1"
				},
				{
				  "sort_key": 200,
				  "name": "P2",
				  "visibility_values": [],
				  "sortkey": 200
				},
				{
				  "sort_key": 300,
				  "visibility_values": [],
				  "name": "P3",
				  "sortkey": 300
				},
				{
				  "sort_key": 400,
				  "name": "P4",
				  "visibility_values": [],
				  "sortkey": 400
				}
			  ],
			  "visibility_values": [],
			  "visibility_field": null,
			  "is_on_bug_entry": false,
			  "is_custom": false,
			  "id": 13
			},
			{
			  "display_name": "Severity",
			  "name": "severity",
			  "type": 2,
			  "is_mandatory": false,
			  "value_field": null,
			  "values": [
				{
				  "sortkey": 100,
				  "sort_key": 100,
				  "visibility_values": [],
				  "name": "Nitpick"
				},
				{
				  "sort_key": 200,
				  "name": "Minor",
				  "visibility_values": [],
				  "sortkey": 200
				},
				{
				  "sort_key": 300,
				  "visibility_values": [],
				  "name": "Critical",
				  "sortkey": 300
				},
				{
				  "sort_key": 400,
				  "name": "Blocker",
				  "visibility_values": [],
				  "sortkey": 400
				}
			  ],
			  "visibility_values": [],
			  "visibility_field": null,
			  "is_on_bug_entry": false,
			  "is_custom": false,
			  "id": 14
			}
		  ]
		}
	)");

	helper.api.process_packet(configure, helper.output);

	CHECK(helper.curl.requestResponse[0].request == "0.0.0.0/rest/field/bug");

	auto root = TaskInfoMessage(TaskID(1), NO_PARENT, "Bugzilla");
	root.createTime = 1737344039870ms;
	root.newTask = true;

	auto p1 = TaskInfoMessage(TaskID(2), TaskID(1), "P1");
	auto p2 = TaskInfoMessage(TaskID(7), TaskID(1), "P2");
	auto p3 = TaskInfoMessage(TaskID(12), TaskID(1), "P3");
	auto p4 = TaskInfoMessage(TaskID(17), TaskID(1), "P4");

	auto p1_nitpick = TaskInfoMessage(TaskID(3), TaskID(2), "Nitpick");
	auto p1_minor = TaskInfoMessage(TaskID(4), TaskID(2), "Minor");
	auto p1_critical = TaskInfoMessage(TaskID(5), TaskID(2), "Critical");
	auto p1_blocker = TaskInfoMessage(TaskID(6), TaskID(2), "Blocker");

	auto p2_nitpick = TaskInfoMessage(TaskID(8), TaskID(7), "Nitpick");
	auto p2_minor = TaskInfoMessage(TaskID(9), TaskID(7), "Minor");
	auto p2_critical = TaskInfoMessage(TaskID(10), TaskID(7), "Critical");
	auto p2_blocker = TaskInfoMessage(TaskID(11), TaskID(7), "Blocker");

	auto p3_nitpick = TaskInfoMessage(TaskID(13), TaskID(12), "Nitpick");
	auto p3_minor = TaskInfoMessage(TaskID(14), TaskID(12), "Minor");
	auto p3_critical = TaskInfoMessage(TaskID(15), TaskID(12), "Critical");
	auto p3_blocker = TaskInfoMessage(TaskID(16), TaskID(12), "Blocker");

	auto p4_nitpick = TaskInfoMessage(TaskID(18), TaskID(17), "Nitpick");
	auto p4_minor = TaskInfoMessage(TaskID(19), TaskID(17), "Minor");
	auto p4_critical = TaskInfoMessage(TaskID(20), TaskID(17), "Critical");
	auto p4_blocker = TaskInfoMessage(TaskID(21), TaskID(17), "Blocker");

	const auto setup_task = [](TaskInfoMessage& task, std::chrono::milliseconds create_time)
		{
			task.createTime = create_time;
			task.newTask = true;
			task.serverControlled = true;
		};

	setup_task(p1, 1737345839870ms);
	setup_task(p2, 1737354839870ms);
	setup_task(p3, 1737363839870ms);
	setup_task(p4, 1737372839870ms);

	setup_task(p1_nitpick, 1737347639870ms);
	setup_task(p1_minor, 1737349439870ms);
	setup_task(p1_critical, 1737351239870ms);
	setup_task(p1_blocker, 1737353039870ms);

	setup_task(p2_nitpick, 1737356639870ms);
	setup_task(p2_minor, 1737358439870ms);
	setup_task(p2_critical, 1737360239870ms);
	setup_task(p2_blocker, 1737362039870ms);

	setup_task(p3_nitpick, 1737365639870ms);
	setup_task(p3_minor, 1737367439870ms);
	setup_task(p3_critical, 1737369239870ms);
	setup_task(p3_blocker, 1737371039870ms);

	setup_task(p4_nitpick, 1737374639870ms);
	setup_task(p4_minor, 1737376439870ms);
	setup_task(p4_critical, 1737378239870ms);
	setup_task(p4_blocker, 1737380039870ms);

	helper.required_messages(
		{
			&root,
			&p1,
			&p1_nitpick, &p1_minor, &p1_critical, &p1_blocker,
			&p2,
			&p2_nitpick, &p2_minor, &p2_critical, &p2_blocker,
			&p3,
			&p3_nitpick, &p3_minor, &p3_critical, &p3_blocker,
			&p4,
			&p4_nitpick, &p4_minor, &p4_critical, &p4_blocker
		});

	SECTION("Reconfigure Does Not Create New Tasks")
	{
		helper.clear_message_output();

		helper.curl.current = 0;
		helper.api.process_packet(configure, helper.output);

		helper.required_messages({});
	}

	SECTION("Group By Task as Strings")
	{
		helper.curl.requestResponse.clear();
		helper.curl.current = 0;

		const auto refresh = RequestMessage(PacketType::BUGZILLA_REFRESH, helper.next_request_id());

		helper.curl.requestResponse.emplace_back("{ \"bugs\": [ { \"id\": 50, \"summary\": \"bug 1\", \"status\": \"Assigned\", \"priority\": \"P2\", \"severity\": \"Minor\" },"
			"{ \"id\": 55, \"summary\": \"bug 2\", \"status\": \"Changes Made\", \"priority\": \"P2\", \"severity\": \"Minor\" },"
			"{ \"id\": 60, \"summary\": \"bug 3\", \"status\": \"Changes Made\", \"priority\": \"P1\", \"severity\": \"Critical\" },"
			"{ \"id\": 65, \"summary\": \"bug 4\", \"status\": \"Reviewed\", \"priority\": \"P3\", \"severity\": \"Blocker\" },"
			"{ \"id\": 70, \"summary\": \"bug 5\", \"status\": \"Confirmed\", \"priority\": \"P4\", \"severity\": \"Nitpick\" } ] }");

		helper.expect_success(refresh);

		CHECK(helper.curl.requestResponse[0].request == "0.0.0.0/rest/bug?assigned_to=test&resolution=---&api_key=asfesdFEASfslj");

		auto taskInfo22 = TaskInfoMessage(TaskID(22), TaskID(9), "50 - bug 1");
		auto taskInfo23 = TaskInfoMessage(TaskID(23), TaskID(9), "55 - bug 2");
		auto taskInfo24 = TaskInfoMessage(TaskID(24), TaskID(5), "60 - bug 3");
		auto taskInfo25 = TaskInfoMessage(TaskID(25), TaskID(16), "65 - bug 4");
		auto taskInfo26 = TaskInfoMessage(TaskID(26), TaskID(18), "70 - bug 5");

		setup_task(taskInfo22, 1737382739870ms);
		setup_task(taskInfo23, 1737384539870ms);
		setup_task(taskInfo24, 1737386339870ms);
		setup_task(taskInfo25, 1737388139870ms);
		setup_task(taskInfo26, 1737389939870ms);

		helper.required_messages(
			{
				&taskInfo22, &taskInfo23, &taskInfo24, &taskInfo25, &taskInfo26
			});

		SECTION("Refreshing Again Does Not Add New Tasks")
		{
			helper.clear_message_output();
			helper.curl.current = 0;

			helper.expect_success(refresh);

			CHECK(helper.curl.requestResponse[0].request == "0.0.0.0/rest/bug?assigned_to=test&resolution=---&api_key=asfesdFEASfslj&last_change_time=2025-01-20T16:48:59Z");

			helper.required_messages({});
		}
	}

	SECTION("Group By Task as Array")
	{
		helper.curl.requestResponse.clear();
		helper.curl.current = 0;

		const auto refresh = RequestMessage(PacketType::BUGZILLA_REFRESH, helper.next_request_id());

		helper.curl.requestResponse.emplace_back("{ \"bugs\": [ { \"id\": 50, \"summary\": \"bug 1\", \"status\": \"Assigned\", \"priority\": \"P2\", \"severity\": [ \"Minor\" ] },"
						"{ \"id\": 55, \"summary\": \"bug 2\", \"status\": \"Changes Made\", \"priority\": \"P2\", \"severity\": [ \"Minor\" ] },"
						"{ \"id\": 60, \"summary\": \"bug 3\", \"status\": \"Changes Made\", \"priority\": \"P1\", \"severity\": [ \"Critical\" ] },"
						"{ \"id\": 65, \"summary\": \"bug 4\", \"status\": \"Reviewed\", \"priority\": \"P3\", \"severity\": [ \"Blocker\" ] },"
						"{ \"id\": 70, \"summary\": \"bug 5\", \"status\": \"Confirmed\", \"priority\": \"P4\", \"severity\": [ \"Nitpick\" ] } ] }");
			
		helper.expect_success(refresh);

		CHECK(helper.curl.requestResponse[0].request == "0.0.0.0/rest/bug?assigned_to=test&resolution=---&api_key=asfesdFEASfslj");

		auto taskInfo22 = TaskInfoMessage(TaskID(22), TaskID(9), "50 - bug 1");
		auto taskInfo23 = TaskInfoMessage(TaskID(23), TaskID(9), "55 - bug 2");
		auto taskInfo24 = TaskInfoMessage(TaskID(24), TaskID(5), "60 - bug 3");
		auto taskInfo25 = TaskInfoMessage(TaskID(25), TaskID(16), "65 - bug 4");
		auto taskInfo26 = TaskInfoMessage(TaskID(26), TaskID(18), "70 - bug 5");

		setup_task(taskInfo22, 1737382739870ms);
		setup_task(taskInfo23, 1737384539870ms);
		setup_task(taskInfo24, 1737386339870ms);
		setup_task(taskInfo25, 1737388139870ms);
		setup_task(taskInfo26, 1737389939870ms);

		helper.required_messages(
			{
				&taskInfo22, &taskInfo23, &taskInfo24, &taskInfo25, &taskInfo26
			});
	}
}






//
//TEST_CASE("Initial Bugzilla Refresh Pulls All Open Bugs", "[bugzilla][api]")
//{
//	TestHelper helper;
//	helper.clock.auto_increment_test_time = false;
//
//	helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 1"));
//
//	// send bugzilla packet
//	auto configure = BugzillaInfoMessage("bugzilla", "0.0.0.0", "asfesdFEASfslj");
//	configure.username = "test";
//	configure.rootTaskID = TaskID(1);
//	configure.groupTasksBy = "severity";
//
//	configure.labelToField["Priority"] = "priority";
//	configure.labelToField["Status"] = "status";
//
//	helper.api.process_packet(configure, helper.output);
//
//	auto minor = TaskInfoMessage(TaskID(2), TaskID(1), "Minor");
//	auto taskInfo3 = TaskInfoMessage(TaskID(3), TaskID(2), "50 - bug 1");
//	auto taskInfo4 = TaskInfoMessage(TaskID(4), TaskID(2), "55 - bug 2");
//	auto critical = TaskInfoMessage(TaskID(5), TaskID(1), "Critical");
//	auto taskInfo6 = TaskInfoMessage(TaskID(6), TaskID(5), "60 - bug 3");
//	auto blocker = TaskInfoMessage(TaskID(7), TaskID(1), "Blocker");
//	auto taskInfo8 = TaskInfoMessage(TaskID(8), TaskID(7), "65 - bug 4");
//	auto nitpick = TaskInfoMessage(TaskID(9), TaskID(1), "Nitpick");
//	auto taskInfo10 = TaskInfoMessage(TaskID(10), TaskID(9), "70 - bug 5");
//
//	minor.createTime = std::chrono::milliseconds(1737344039870);
//	taskInfo3.createTime = std::chrono::milliseconds(1737344039870);
//	taskInfo4.createTime = std::chrono::milliseconds(1737344039870);
//	critical.createTime = std::chrono::milliseconds(1737344039870);
//	taskInfo6.createTime = std::chrono::milliseconds(1737344039870);
//	blocker.createTime = std::chrono::milliseconds(1737344039870);
//	taskInfo8.createTime = std::chrono::milliseconds(1737344039870);
//	nitpick.createTime = std::chrono::milliseconds(1737344039870);
//	taskInfo10.createTime = std::chrono::milliseconds(1737344039870);
//	minor.newTask = true;
//	taskInfo3.newTask = true;
//	taskInfo4.newTask = true;
//	critical.newTask = true;
//	taskInfo6.newTask = true;
//	blocker.newTask = true;
//	taskInfo8.newTask = true;
//	nitpick.newTask = true;
//	taskInfo10.newTask = true;
//	minor.serverControlled = true;
//	taskInfo3.serverControlled = true;
//	taskInfo4.serverControlled = true;
//	critical.serverControlled = true;
//	taskInfo6.serverControlled = true;
//	blocker.serverControlled = true;
//	taskInfo8.serverControlled = true;
//	nitpick.serverControlled = true;
//	taskInfo10.serverControlled = true;
//
//	SECTION("Group By Task As String")
//	{
//		// send bugzilla refresh packet
//		const auto refresh = RequestMessage(PacketType::BUGZILLA_REFRESH, helper.next_request_id());
//
//		helper.curl.result = "{ \"bugs\": [ { \"id\": 50, \"summary\": \"bug 1\", \"status\": \"Assigned\", \"priority\": \"Normal\", \"severity\": \"Minor\" },"
//			"{ \"id\": 55, \"summary\": \"bug 2\", \"status\": \"Changes Made\", \"priority\": \"Normal\", \"severity\": \"Minor\" },"
//			"{ \"id\": 60, \"summary\": \"bug 3\", \"status\": \"Changes Made\", \"priority\": \"Low\", \"severity\": \"Critical\" },"
//			"{ \"id\": 65, \"summary\": \"bug 4\", \"status\": \"Reviewed\", \"priority\": \"High\", \"severity\": \"Blocker\" },"
//			"{ \"id\": 70, \"summary\": \"bug 5\", \"status\": \"Confirmed\", \"priority\": \"Highest\", \"severity\": \"Nitpick\" } ] }";
//
//		helper.expect_success(refresh);
//
//		CHECK(helper.curl.request == "0.0.0.0/rest/bug?assigned_to=test&resolution=---&api_key=asfesdFEASfslj");
//
//		helper.required_messages({ &minor, &taskInfo3, &taskInfo4, &critical, &taskInfo6, &blocker, &taskInfo8, &nitpick, &taskInfo10 });
//	}
//	
//	SECTION("Group By Task As Array of Strings")
//	{
//		// send bugzilla refresh packet
//		const auto refresh = RequestMessage(PacketType::BUGZILLA_REFRESH, helper.next_request_id());
//
//		helper.curl.result = "{ \"bugs\": [ { \"id\": 50, \"summary\": \"bug 1\", \"status\": \"Assigned\", \"priority\": \"Normal\", \"severity\": [ \"Minor\" ] },"
//			"{ \"id\": 55, \"summary\": \"bug 2\", \"status\": \"Changes Made\", \"priority\": \"Normal\", \"severity\": [ \"Minor\" ] },"
//			"{ \"id\": 60, \"summary\": \"bug 3\", \"status\": \"Changes Made\", \"priority\": \"Low\", \"severity\": [ \"Critical\" ] },"
//			"{ \"id\": 65, \"summary\": \"bug 4\", \"status\": \"Reviewed\", \"priority\": \"High\", \"severity\": [ \"Blocker\" ] },"
//			"{ \"id\": 70, \"summary\": \"bug 5\", \"status\": \"Confirmed\", \"priority\": \"Highest\", \"severity\": [ \"Nitpick\" ] } ] }";
//
//		helper.expect_success(refresh);
//
//		CHECK(helper.curl.request == "0.0.0.0/rest/bug?assigned_to=test&resolution=---&api_key=asfesdFEASfslj");
//
//		helper.required_messages({ &minor, &taskInfo3, &taskInfo4, &critical, &taskInfo6, &blocker, &taskInfo8, &nitpick, &taskInfo10 });
//	}
//}
//
//TEST_CASE("Subsequent Bugzilla Refresh Requests Updates Since Last Refresh", "[bugzilla][api]")
//{
//	TestHelper helper;
//	helper.clock.auto_increment_test_time = false;
//
//	helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 1"));
//
//	// send bugzilla packet
//	auto configure = BugzillaInfoMessage("bugzilla", "0.0.0.0", "asfesdFEASfslj");
//	configure.username = "test";
//	configure.rootTaskID = TaskID(1);
//	configure.groupTasksBy = "severity";
//
//	configure.labelToField["Priority"] = "priority";
//	configure.labelToField["Status"] = "status";
//
//	helper.api.process_packet(configure, helper.output);
//
//	// send bugzilla refresh packet
//	const auto refresh = RequestMessage(PacketType::BUGZILLA_REFRESH, helper.next_request_id());
//
//	helper.curl.result = "{ \"bugs\": [ { \"id\": 50, \"summary\": \"bug 1\", \"status\": \"Assigned\", \"priority\": \"Normal\", \"severity\": \"Minor\" },"
//		"{ \"id\": 55, \"summary\": \"bug 2\", \"status\": \"Changes Made\", \"priority\": \"Normal\", \"severity\": \"Major\" },"
//		"{ \"id\": 60, \"summary\": \"bug 3\", \"status\": \"Changes Made\", \"priority\": \"Low\", \"severity\": \"Critical\" },"
//		"{ \"id\": 65, \"summary\": \"bug 4\", \"status\": \"Reviewed\", \"priority\": \"High\", \"severity\": \"Blocker\" },"
//		"{ \"id\": 70, \"summary\": \"bug 5\", \"status\": \"Confirmed\", \"priority\": \"Highest\", \"severity\": \"Nitpick\" },"
//		"{ \"id\": 75, \"summary\": \"bug 6\", \"status\": \"Confirmed\", \"priority\": \"Highest\", \"severity\": \"Minor\" } ] }";
//
//	helper.expect_success(refresh);
//	helper.clock.time += std::chrono::hours(2);
//	helper.output.clear();
//
//	
//
//	// verify various operations
//	// new tasks are added
//	// tasks are reparented if their grouping changes
//	// tasks are finished if the bug is resolved
//	// labels are added and removed
//
//	// TODO We should finish the grouping tasks if they are unused. Then we can unfinish them if required to
//
//	SECTION("Last Change Time")
//	{
//		helper.curl.result = "{ \"bugs\": [] }";
//
//		helper.expect_success(refresh);
//
//		// verify curl request has previous refresh date
//		CHECK(helper.curl.request == "0.0.0.0/rest/bug?assigned_to=test&resolution=---&api_key=asfesdFEASfslj&last_change_time=2025-01-20T05:33:59Z");
//	}
//
//	SECTION("Bug Summary Change")
//	{
//		helper.curl.result = "{ \"bugs\": [ { \"id\": 50, \"summary\": \"bug 1 name change\", \"status\": \"Assigned\", \"priority\": \"Normal\", \"severity\": \"Minor\" } ] }";
//
//		helper.expect_success(refresh);
//
//		auto taskInfo3 = TaskInfoMessage(TaskID(3), TaskID(2), "50 - bug 1 name change");
//		taskInfo3.createTime = std::chrono::milliseconds(1737344039870);
//		taskInfo3.serverControlled = true;
//
//		helper.required_messages({ &taskInfo3 });
//	}
//
//	SECTION("New Bug")
//	{
//		helper.curl.result = "{ \"bugs\": [ { \"id\": 80, \"summary\": \"new bug\", \"status\": \"Assigned\", \"priority\": \"Normal\", \"severity\": \"Minor\" } ] }";
//
//		helper.expect_success(refresh);
//
//		auto taskInfo13 = TaskInfoMessage(TaskID(13), TaskID(2), "80 - new bug");
//		taskInfo13.createTime = std::chrono::milliseconds(1737351239870);
//		taskInfo13.newTask = true;
//		taskInfo13.serverControlled = true;
//
//		helper.required_messages({ &taskInfo13 });
//	}
//
//	SECTION("Reparent When Group By Field Changes")
//	{
//		helper.curl.result = "{ \"bugs\": [ { \"id\": 50, \"summary\": \"bug 1\", \"status\": \"Assigned\", \"priority\": \"Normal\", \"severity\": \"Major\" } ] }";
//
//		helper.expect_success(refresh);
//
//		auto taskInfo3 = TaskInfoMessage(TaskID(3), TaskID(4), "50 - bug 1");
//		taskInfo3.createTime = std::chrono::milliseconds(1737344039870);
//		taskInfo3.serverControlled = true;
//
//		helper.required_messages({ &taskInfo3 });
//	}
//
//	SECTION("Finish Parent Task When It Has No Children")
//	{
//		helper.curl.result = "{ \"bugs\": [ { \"id\": 55, \"summary\": \"bug 2\", \"status\": \"Changes Made\", \"priority\": \"Normal\", \"severity\": \"Critical\" } ] }";
//
//		helper.expect_success(refresh);
//
//		auto taskInfo3 = TaskInfoMessage(TaskID(5), TaskID(6), "55 - bug 2");
//		taskInfo3.createTime = std::chrono::milliseconds(1737344039870);
//		taskInfo3.serverControlled = true;
//
//		auto major = TaskInfoMessage(TaskID(4), TaskID(1), "Major");
//		major.createTime = std::chrono::milliseconds(1737344039870);
//		major.state = TaskState::FINISHED;
//		major.finishTime = std::chrono::milliseconds(1737351239870);
//		major.serverControlled = true;
//
//		helper.required_messages({ &taskInfo3, &major });
//
//		SECTION("Return Parent Task to Active When It Has Children")
//		{
//			helper.curl.result = "{ \"bugs\": [ { \"id\": 55, \"summary\": \"bug 2\", \"status\": \"Changes Made\", \"priority\": \"Normal\", \"severity\": \"Major\" } ] }";
//
//			helper.expect_success(refresh);
//
//			taskInfo3 = TaskInfoMessage(TaskID(5), TaskID(4), "55 - bug 2");
//			taskInfo3.createTime = std::chrono::milliseconds(1737344039870);
//			taskInfo3.serverControlled = true;
//
//			major.state = TaskState::INACTIVE;
//			
//			helper.required_messages({ &taskInfo3, &major });
//		}
//	}
//
//	SECTION("Create New Parent Task and Reparent When Group By Field Changes to Previously Unseen Value")
//	{
//		helper.curl.result = "{ \"bugs\": [ { \"id\": 50, \"summary\": \"bug 1 name change\", \"status\": \"Assigned\", \"priority\": \"Normal\", \"severity\": \"New\" } ] }";
//
//		helper.expect_success(refresh);
//
//		auto newTask = TaskInfoMessage(TaskID(13), TaskID(1), "New");
//		newTask.createTime = std::chrono::milliseconds(1737351239870);
//		newTask.newTask = true;
//		newTask.serverControlled = true;
//
//		auto taskInfo3 = TaskInfoMessage(TaskID(3), TaskID(13), "50 - bug 1 name change");
//		taskInfo3.createTime = std::chrono::milliseconds(1737344039870);
//		taskInfo3.serverControlled = true;
//
//		helper.required_messages({ &newTask, &taskInfo3 });
//	}
//}
//
