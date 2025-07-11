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

	helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "Bugzilla"));
	helper.clear_message_output();

	// send bugzilla packet
	auto configure = BugzillaInfoMessage("bugzilla", "0.0.0.0", "asfesdFEASfslj");
	configure.username = "test";
	configure.rootTaskID = TaskID(1);
	configure.groupTasksBy.push_back("product");
	configure.groupTasksBy.push_back("severity");

	configure.labelToField["Priority"] = "priority";
	configure.labelToField["Status"] = "status";

	helper.curl.requestResponse.emplace_back("{ \"fields\": [] }");
	helper.curl.requestResponse.emplace_back("{ \"bugs\": [] }");

	helper.api.process_packet(configure, helper.output);

	helper.required_messages({ &configure });
}

TEST_CASE("Request Bugzilla Information", "[bugzilla][api]")
{
	TestHelper helper;

	helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "Bugzilla"));
	helper.clear_message_output();

	// send bugzilla packet
	auto configure = BugzillaInfoMessage("bugzilla", "0.0.0.0", "asfesdFEASfslj");
	configure.username = "test";
	configure.rootTaskID = TaskID(1);
	configure.groupTasksBy.push_back("product");
	configure.groupTasksBy.push_back("severity");

	configure.labelToField["Priority"] = "priority";
	configure.labelToField["Status"] = "status";

	helper.curl.requestResponse.emplace_back("{ \"fields\": [] }");
	helper.curl.requestResponse.emplace_back("{ \"bugs\": [] }");

	helper.api.process_packet(configure, helper.output);
	helper.clear_message_output();

	auto request = BasicMessage(PacketType::REQUEST_CONFIGURATION);

	helper.api.process_packet(request, helper.output);

	auto timeCategories = TimeEntryDataPacket({});
	auto complete = BasicMessage(PacketType::REQUEST_CONFIGURATION_COMPLETE);

	auto root = TaskInfoMessage(TaskID(1), NO_PARENT, "Bugzilla");
	root.createTime = std::chrono::milliseconds(1737344039870);

	helper.required_messages({ &timeCategories, &root, &configure, &complete });
}

TEST_CASE("Configuring Multiple Bugzilla Instances", "[bugzilla][api]")
{
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

	const std::string fieldsResponse = R"bugs_data(
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
	)bugs_data";

	helper.curl.requestResponse.emplace_back(fieldsResponse);
	helper.curl.requestResponse.emplace_back("{ \"bugs\": [] }");
	helper.curl.requestResponse.emplace_back(fieldsResponse);
	helper.curl.requestResponse.emplace_back("{ \"bugs\": [] }");

	helper.api.process_packet(configure, helper.output);

	auto configure2 = configure;
	configure2.name = "bugzilla2";

	helper.curl.requestResponse.clear();
	helper.curl.requestResponse.emplace_back(fieldsResponse);
	helper.curl.requestResponse.emplace_back("{ \"bugs\": [] }");
	helper.curl.requestResponse.emplace_back("{ \"bugs\": [] }");
	helper.curl.current = 0;

	helper.api.process_packet(configure2, helper.output);

	SECTION("Information is Set in Memory")
	{
		helper.clear_message_output();

		auto request = BasicMessage(PacketType::REQUEST_CONFIGURATION);

		helper.api.process_packet(request, helper.output);

		auto timeCategories = TimeEntryDataPacket({});
		auto complete = BasicMessage(PacketType::REQUEST_CONFIGURATION_COMPLETE);

		auto root = TaskInfoMessage(TaskID(1), NO_PARENT, "Bugzilla");
		root.createTime = std::chrono::milliseconds(1737344039870);

		helper.required_messages({ &timeCategories, &root, &configure, &configure2, &complete });
	}
}

// TODO this is all temporary. we need something setup to use, this will have to do. persistence will just be a log of steps to rebuild our data
TEST_CASE("Bugzilla Persistence", "[bugzilla][api]")
{
	TestHelper helper;

	const std::string expectedOutput = R"expected_output(create 1 0 1737344939870 0 (6 test 1)
task-time-codes 1 0 0 
bugzilla-config bugzilla 0.0.0.0 asfesdFEASfslj
test
1
2
priority
severity
2
Priority
priority
Status
status
bugzilla-refresh bugzilla 1737345839870
bugzilla-tasks bugzilla 
create 2 1 1737348539870 1 (2 P2)
create 3 2 1737350339870 1 (5 Minor)
unfinish 3
unfinish 2
unfinish 1
create 4 3 1737352139870 1 (10 50 - bug 1)
unfinish 3
unfinish 2
unfinish 1
create 5 3 1737353939870 1 (10 55 - bug 2)
create 6 1 1737355739870 1 (2 P1)
create 7 6 1737357539870 1 (8 Critical)
unfinish 7
unfinish 6
unfinish 1
create 8 7 1737359339870 1 (10 60 - bug 3)
create 9 1 1737361139870 1 (2 P3)
create 10 9 1737362939870 1 (7 Blocker)
unfinish 10
unfinish 9
unfinish 1
create 11 10 1737364739870 1 (10 65 - bug 4)
create 12 1 1737366539870 1 (2 P4)
create 13 12 1737368339870 1 (7 Nitpick)
unfinish 13
unfinish 12
unfinish 1
create 14 13 1737370139870 1 (10 70 - bug 5)
bugzilla-refresh bugzilla 1737346739870
bugzilla-tasks bugzilla 50 4 55 5 60 8 65 11 70 14 
bugzilla-refresh bugzilla 1737378239870
bugzilla-tasks bugzilla 50 4 55 5 60 8 65 11 70 14 
)expected_output";

	SECTION("Save")
	{
		auto create1 = CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 1");
		helper.expect_success(create1);

		auto configure = BugzillaInfoMessage("bugzilla", "0.0.0.0", "asfesdFEASfslj");
		configure.username = "test";
		configure.rootTaskID = TaskID(1);
		configure.groupTasksBy.push_back("priority");
		configure.groupTasksBy.push_back("severity");

		configure.labelToField["Priority"] = "priority";
		configure.labelToField["Status"] = "status";

		const std::string fieldsResponse = R"bugs_data(
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
	)bugs_data";

		helper.curl.requestResponse.emplace_back(fieldsResponse);
		helper.curl.requestResponse.emplace_back("{ \"bugs\": [] }");

		helper.api.process_packet(configure, helper.output);

		const auto refresh = RequestMessage(PacketType::BUGZILLA_REFRESH, helper.next_request_id());

		helper.curl.requestResponse.emplace_back("{ \"bugs\": [ { \"id\": 50, \"summary\": \"bug 1\", \"status\": \"Assigned\", \"priority\": \"P2\", \"severity\": \"Minor\" },"
			"{ \"id\": 55, \"summary\": \"bug 2\", \"status\": \"Changes Made\", \"priority\": \"P2\", \"severity\": \"Minor\" },"
			"{ \"id\": 60, \"summary\": \"bug 3\", \"status\": \"Changes Made\", \"priority\": \"P1\", \"severity\": \"Critical\" },"
			"{ \"id\": 65, \"summary\": \"bug 4\", \"status\": \"Reviewed\", \"priority\": \"P3\", \"severity\": \"Blocker\" },"
			"{ \"id\": 70, \"summary\": \"bug 5\", \"status\": \"Confirmed\", \"priority\": \"P4\", \"severity\": \"Nitpick\" } ] }");

		helper.expect_success(refresh);
		helper.curl.current = 1;
		helper.clock.time += std::chrono::hours(2);
		helper.expect_success(refresh);

		const auto actual = helper.fileOutput.str();

		CHECK(actual == expectedOutput);
	}

	SECTION("Load")
	{
		helper.fileOutput << expectedOutput;
		helper.fileInput = std::istringstream(helper.fileOutput.str());
		helper.fileOutput.clear();

		helper.api = API(helper.clock, helper.curl, helper.fileInput, helper.fileOutput);

		// now that we're setup, request the configuration and check the output
		helper.api.process_packet(BasicMessage{ PacketType::REQUEST_CONFIGURATION }, helper.output);

		auto configure = BugzillaInfoMessage("bugzilla", "0.0.0.0", "asfesdFEASfslj");
		configure.username = "test";
		configure.rootTaskID = TaskID(1);
		configure.groupTasksBy.push_back("priority");
		configure.groupTasksBy.push_back("severity");

		configure.labelToField["Priority"] = "priority";
		configure.labelToField["Status"] = "status";

		auto timeCategories = TimeEntryDataPacket({});
		auto complete = BasicMessage(PacketType::REQUEST_CONFIGURATION_COMPLETE);

		auto p1 = TaskInfoMessage(TaskID(6), TaskID(1), "P1");
		auto p2 = TaskInfoMessage(TaskID(2), TaskID(1), "P2");
		auto p3 = TaskInfoMessage(TaskID(9), TaskID(1), "P3");
		auto p4 = TaskInfoMessage(TaskID(12), TaskID(1), "P4");

		auto p1_critical = TaskInfoMessage(TaskID(7), TaskID(6), "Critical");
		auto p2_minor = TaskInfoMessage(TaskID(3), TaskID(2), "Minor");
		auto p3_blocker = TaskInfoMessage(TaskID(10), TaskID(9), "Blocker");
		auto p4_nitpick = TaskInfoMessage(TaskID(13), TaskID(12), "Nitpick");

		const auto setup_task_inactive = [](TaskInfoMessage& task, std::chrono::milliseconds create_time)
			{
				task.createTime = create_time;
				task.finishTime = std::nullopt;
				task.state = TaskState::INACTIVE;
				task.serverControlled = true;
			};

		const auto setup_task_finished = [](TaskInfoMessage& task, std::chrono::milliseconds create_time, std::chrono::milliseconds finishTime)
			{
				task.createTime = create_time;
				task.finishTime = finishTime;
				task.state = TaskState::FINISHED;
				task.serverControlled = true;
			};

		using namespace std::chrono_literals;

		auto root = TaskInfoMessage(TaskID(1), NO_PARENT, "test 1");
		root.createTime = 1737344939870ms;

		setup_task_inactive(p1, 1737355739870ms);
		setup_task_inactive(p2, 1737348539870ms);
		setup_task_inactive(p3, 1737361139870ms);
		setup_task_inactive(p4, 1737366539870ms);

		setup_task_inactive(p1_critical, 1737357539870ms);
		setup_task_inactive(p2_minor, 1737350339870ms);
		setup_task_inactive(p3_blocker, 1737362939870ms);
		setup_task_inactive(p4_nitpick, 1737368339870ms);

		auto taskInfo4 = TaskInfoMessage(TaskID(4), TaskID(3), "50 - bug 1");
		auto taskInfo5 = TaskInfoMessage(TaskID(5), TaskID(3), "55 - bug 2");
		auto taskInfo8 = TaskInfoMessage(TaskID(8), TaskID(7), "60 - bug 3");
		auto taskInfo11 = TaskInfoMessage(TaskID(11), TaskID(10), "65 - bug 4");
		auto taskInfo14 = TaskInfoMessage(TaskID(14), TaskID(13), "70 - bug 5");

		setup_task_inactive(taskInfo4, 1737352139870ms);
		setup_task_inactive(taskInfo5, 1737353939870ms);
		setup_task_inactive(taskInfo8, 1737359339870ms);
		setup_task_inactive(taskInfo11, 1737364739870ms);
		setup_task_inactive(taskInfo14, 1737370139870ms);

		helper.required_messages(
			{
				&timeCategories,

				&root,
				
				&p2, &p2_minor, &taskInfo4, &taskInfo5,
				&p1, &p1_critical, &taskInfo8,
				&p3, &p3_blocker, &taskInfo11,
				&p4, &p4_nitpick, &taskInfo14,

				&configure, &complete,
			});
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
	const std::string fieldsResponse = R"bugs_data(
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
	)bugs_data";

	helper.curl.requestResponse.emplace_back(fieldsResponse);
	helper.curl.requestResponse.emplace_back("{ \"bugs\": [] }");

	helper.api.process_packet(configure, helper.output);

	CHECK(helper.curl.requestResponse[0].request == "0.0.0.0/rest/field/bug?api_key=asfesdFEASfslj");

	auto root = TaskInfoMessage(TaskID(1), NO_PARENT, "Bugzilla");
	root.createTime = 1737344039870ms;
	root.newTask = true;

	/*auto p1 = TaskInfoMessage(TaskID(2), TaskID(1), "P1");
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
	auto p4_blocker = TaskInfoMessage(TaskID(21), TaskID(17), "Blocker");*/

	const auto setup_task_inactive = [](TaskInfoMessage& task, std::chrono::milliseconds create_time)
		{
			task.createTime = create_time;
			task.finishTime = std::nullopt;
			task.state = TaskState::INACTIVE;
			task.newTask = true;
			task.serverControlled = true;
		};

	const auto setup_task_finished = [](TaskInfoMessage& task, std::chrono::milliseconds create_time, std::chrono::milliseconds finishTime)
		{
			task.createTime = create_time;
			task.finishTime = finishTime;
			task.state = TaskState::FINISHED;
			task.newTask = true;
			task.serverControlled = true;
		};

	/*setup_task_finished(p1, 1737345839870ms, 1737381839870ms);
	setup_task_finished(p2, 1737354839870ms, 1737390839870ms);
	setup_task_finished(p3, 1737363839870ms, 1737399839870ms);
	setup_task_finished(p4, 1737372839870ms, 1737408839870ms);

	setup_task_finished(p1_nitpick, 1737347639870ms, 1737383639870ms);
	setup_task_finished(p1_minor, 1737349439870ms, 1737385439870ms);
	setup_task_finished(p1_critical, 1737351239870ms, 1737387239870ms);
	setup_task_finished(p1_blocker, 1737353039870ms, 1737389039870ms);

	setup_task_finished(p2_nitpick, 1737356639870ms, 1737392639870ms);
	setup_task_finished(p2_minor, 1737358439870ms, 1737394439870ms);
	setup_task_finished(p2_critical, 1737360239870ms, 1737396239870ms);
	setup_task_finished(p2_blocker, 1737362039870ms, 1737398039870ms);

	setup_task_finished(p3_nitpick, 1737365639870ms, 1737401639870ms);
	setup_task_finished(p3_minor, 1737367439870ms, 1737403439870ms);
	setup_task_finished(p3_critical, 1737369239870ms, 1737405239870ms);
	setup_task_finished(p3_blocker, 1737371039870ms, 1737407039870ms);

	setup_task_finished(p4_nitpick, 1737374639870ms, 1737410639870ms);
	setup_task_finished(p4_minor, 1737376439870ms, 1737412439870ms);
	setup_task_finished(p4_critical, 1737378239870ms, 1737414239870ms);
	setup_task_finished(p4_blocker, 1737380039870ms, 1737416039870ms);*/

	helper.required_messages(
		{
			&root,
			&configure
		});

	SECTION("Reconfigure Does Not Create New Tasks")
	{
		helper.clear_message_output();

		helper.curl.current = 0;
		helper.api.process_packet(configure, helper.output);

		helper.required_messages({ &configure });
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

		CHECK(helper.curl.requestResponse[0].request == "0.0.0.0/rest/bug?assigned_to=test&api_key=asfesdFEASfslj&last_change_time=2025-01-20T04:18:59Z");

		auto p1 = TaskInfoMessage(TaskID(6), TaskID(1), "P1");
		auto p2 = TaskInfoMessage(TaskID(2), TaskID(1), "P2");
		auto p3 = TaskInfoMessage(TaskID(9), TaskID(1), "P3");
		auto p4 = TaskInfoMessage(TaskID(12), TaskID(1), "P4");

		auto p1_critical = TaskInfoMessage(TaskID(7), TaskID(6), "Critical");
		auto p2_minor = TaskInfoMessage(TaskID(3), TaskID(2), "Minor");
		auto p3_blocker = TaskInfoMessage(TaskID(10), TaskID(9), "Blocker");
		auto p4_nitpick = TaskInfoMessage(TaskID(13), TaskID(12), "Nitpick");

		auto taskInfo4 = TaskInfoMessage(TaskID(4), TaskID(3), "50 - bug 1");
		auto taskInfo5 = TaskInfoMessage(TaskID(5), TaskID(3), "55 - bug 2");
		auto taskInfo8 = TaskInfoMessage(TaskID(8), TaskID(7), "60 - bug 3");
		auto taskInfo11 = TaskInfoMessage(TaskID(11), TaskID(10), "65 - bug 4");
		auto taskInfo14 = TaskInfoMessage(TaskID(14), TaskID(13), "70 - bug 5");

		setup_task_inactive(taskInfo4, 1737351239870ms);
		setup_task_inactive(taskInfo5, 1737353039870ms);
		setup_task_inactive(taskInfo8, 1737358439870ms);
		setup_task_inactive(taskInfo11, 1737363839870ms);
		setup_task_inactive(taskInfo14, 1737369239870ms);

		setup_task_inactive(p1, 1737354839870ms);
		setup_task_inactive(p2, 1737347639870ms);
		setup_task_inactive(p3, 1737360239870ms);
		setup_task_inactive(p4, 1737365639870ms);
		setup_task_inactive(p1_critical, 1737356639870ms);
		setup_task_inactive(p2_minor, 1737349439870ms);
		setup_task_inactive(p3_blocker, 1737362039870ms);
		setup_task_inactive(p4_nitpick, 1737367439870ms);

		p1.newTask = p2.newTask = p3.newTask = p4.newTask = p1_critical.newTask = p2_minor.newTask = p3_blocker.newTask = p4_nitpick.newTask = true;

		helper.required_messages(
			{
				&p2, &p2_minor, &taskInfo4, &taskInfo5, 
				&p1, &p1_critical, &taskInfo8, 
				&p3, &p3_blocker, 
				&taskInfo11, 
				&p4, &p4_nitpick,
				&taskInfo14
			});

		SECTION("Refreshing Again Does Not Add New Tasks")
		{
			helper.clear_message_output();
			helper.curl.current = 0;

			helper.expect_success(refresh);

			CHECK(helper.curl.requestResponse[0].request == "0.0.0.0/rest/bug?assigned_to=test&api_key=asfesdFEASfslj&last_change_time=2025-01-20T11:03:59Z");

			helper.required_messages({});
		}

		SECTION("Bug Renames Send Update to UI")
		{
			helper.clear_message_output();

			helper.curl.requestResponse.emplace_back("{ \"bugs\": [ { \"id\": 50, \"summary\": \"bug 1 rename\", \"status\": \"Assigned\", \"priority\": \"P2\", \"severity\": \"Minor\" },"
				"{ \"id\": 55, \"summary\": \"bug 2\", \"status\": \"Changes Made\", \"priority\": \"P2\", \"severity\": \"Minor\" },"
				"{ \"id\": 60, \"summary\": \"bug 3 rename\", \"status\": \"Changes Made\", \"priority\": \"P1\", \"severity\": \"Critical\" },"
				"{ \"id\": 65, \"summary\": \"bug 4\", \"status\": \"Reviewed\", \"priority\": \"P3\", \"severity\": \"Blocker\" },"
				"{ \"id\": 70, \"summary\": \"bug 5 rename\", \"status\": \"Confirmed\", \"priority\": \"P4\", \"severity\": \"Nitpick\" } ] }");

			helper.expect_success(refresh);

			taskInfo4.newTask = false;
			taskInfo4.name = "50 - bug 1 rename";

			taskInfo8.newTask = false;
			taskInfo8.name = "60 - bug 3 rename";

			taskInfo14.newTask = false;
			taskInfo14.name = "70 - bug 5 rename";

			helper.required_messages({ &taskInfo4, &taskInfo8, &taskInfo14 });
		}

		SECTION("New Bugs Are Added")
		{
			helper.clear_message_output();

			helper.curl.requestResponse.emplace_back("{ \"bugs\": [ { \"id\": 75, \"summary\": \"bug 6\", \"status\": \"Confirmed\", \"priority\": \"P4\", \"severity\": \"Nitpick\" } ] }");

			helper.expect_success(refresh);

			auto taskInfo27 = TaskInfoMessage(TaskID(15), TaskID(13), "75 - bug 6");

			setup_task_inactive(taskInfo27, 1737371939870ms);

			helper.required_messages({ &taskInfo27 });
		}

		SECTION("Resolved Bugs Are Finished")
		{
			helper.clear_message_output();

			helper.curl.requestResponse.emplace_back("{ \"bugs\": [ { \"id\": 50, \"summary\": \"bug 1 rename\", \"status\": \"RESOLVED\", \"priority\": \"P2\", \"severity\": \"Minor\" } ] }");

			helper.expect_success(refresh);

			taskInfo4.name = "50 - bug 1 rename";
			taskInfo4.newTask = false;
			taskInfo4.state = TaskState::FINISHED;
			taskInfo4.finishTime = 1737371939870ms;

			helper.required_messages({ &taskInfo4 });
		}

		SECTION("Creating New Grouping Tasks - Moving Bug to New Group By")
		{
			SECTION("Change Priority (First Layer of Grouping)")
			{
				helper.clear_message_output();

				helper.curl.requestResponse.emplace_back("{ \"bugs\": [ { \"id\": 70, \"summary\": \"bug 5\", \"status\": \"Confirmed\", \"priority\": \"P5\", \"severity\": \"Nitpick\" } ] }");

				helper.expect_success(refresh);

				auto p5 = TaskInfoMessage(TaskID(15), TaskID(1), "P5");
				auto p5_nitpick = TaskInfoMessage(TaskID(16), TaskID(15), "Nitpick");

				setup_task_inactive(p5, 1737371939870ms);
				setup_task_inactive(p5_nitpick, 1737373739870ms);
				setup_task_finished(p4, 1737365639870ms, 1737375539870ms);
				setup_task_finished(p4_nitpick, 1737367439870ms, 1737377339870ms);

				taskInfo14.newTask = false;
				taskInfo14.parentID = TaskID(16);

				p4_nitpick.newTask = false;
				p4.newTask = false;

				helper.required_messages({ &p5, &p5_nitpick, &taskInfo14, &p4, &p4_nitpick });
			}

			SECTION("Change Severity (Second Layer of Grouping)")
			{
				helper.clear_message_output();

				helper.curl.requestResponse.emplace_back("{ \"bugs\": [ { \"id\": 70, \"summary\": \"bug 5\", \"status\": \"Confirmed\", \"priority\": \"P4\", \"severity\": \"Minor2\" } ] }");

				helper.expect_success(refresh);

				auto p4_minor2 = TaskInfoMessage(TaskID(15), TaskID(12), "Minor2");

				setup_task_inactive(p4_minor2, 1737371939870ms);
				setup_task_finished(p4_nitpick, 1737367439870ms, 1737373739870ms);

				taskInfo14.newTask = false;
				taskInfo14.parentID = TaskID(15);

				p4_nitpick.newTask = false;

				helper.required_messages({ &p4_minor2, &taskInfo14, &p4_nitpick });
			}
		}

		SECTION("Creating New Grouping Tasks - New Bug in New Group By")
		{
			SECTION("Change Priority (First Layer of Grouping)")
			{
				helper.clear_message_output();

				helper.curl.requestResponse.emplace_back("{ \"bugs\": [ { \"id\": 75, \"summary\": \"bug 6\", \"status\": \"Confirmed\", \"priority\": \"P5\", \"severity\": \"Nitpick\" } ] }");

				helper.expect_success(refresh);

				auto p5 = TaskInfoMessage(TaskID(15), TaskID(1), "P5");
				auto p5_nitpick = TaskInfoMessage(TaskID(16), TaskID(15), "Nitpick");

				setup_task_inactive(p5, 1737371939870ms);
				setup_task_inactive(p5_nitpick, 1737373739870ms);

				taskInfo14.newTask = false;
				taskInfo14.parentID = TaskID(16);

				auto taskInfo29 = TaskInfoMessage(TaskID(17), TaskID(16), "75 - bug 6");

				setup_task_inactive(taskInfo29, 1737375539870ms);

				helper.required_messages({ &p5, &p5_nitpick, &taskInfo29 });
			}

			SECTION("Change Severity (Second Layer of Grouping)")
			{
				helper.clear_message_output();

				helper.curl.requestResponse.emplace_back("{ \"bugs\": [ { \"id\": 75, \"summary\": \"bug 6\", \"status\": \"Confirmed\", \"priority\": \"P4\", \"severity\": \"Minor2\" } ] }");

				helper.expect_success(refresh);

				auto p4_minor2 = TaskInfoMessage(TaskID(15), TaskID(12), "Minor2");

				setup_task_inactive(p4_minor2, 1737371939870ms);

				taskInfo14.newTask = false;
				taskInfo14.parentID = TaskID(15);

				auto taskInfo28 = TaskInfoMessage(TaskID(16), TaskID(15), "75 - bug 6");

				setup_task_inactive(taskInfo28, 1737373739870ms);

				helper.required_messages({ &p4_minor2, &taskInfo28 });
			}
		}

		SECTION("Finishing Old Grouping Tasks - Last Bug is Moved")
		{
			helper.clear_message_output();

			helper.curl.requestResponse.emplace_back("{ \"bugs\": [ { \"id\": 60, \"summary\": \"bug 3\", \"status\": \"Confirmed\", \"priority\": \"P2\", \"severity\": \"Critical\" } ] }");

			helper.expect_success(refresh);

			auto taskInfo24 = TaskInfoMessage(TaskID(8), TaskID(15), "60 - bug 3");
			taskInfo24.serverControlled = 1;
			taskInfo24.createTime = 1737358439870ms;

			p1_critical.state = TaskState::FINISHED;
			p1_critical.finishTime = 1737431339870ms;
			
			//setup_task_inactive(p2_critical, 1737360239870ms);
			//p2_critical.newTask = false;

			setup_task_finished(p1, 1737345839870ms, 1737429539870ms);
			p1.newTask = false;

			helper.required_messages({ &taskInfo24, &p1, &p1_critical/*, &p2_critical*/});
		}

		SECTION("Finishing Old Grouping Tasks - Last Bug is Finished")
		{
			helper.clear_message_output();

			helper.curl.requestResponse.emplace_back("{ \"bugs\": [ { \"id\": 60, \"summary\": \"bug 3\", \"status\": \"RESOLVED\", \"priority\": \"P1\", \"severity\": \"Critical\" } ] }");

			helper.expect_success(refresh);

			auto taskInfo24 = TaskInfoMessage(TaskID(24), TaskID(5), "60 - bug 3");
			setup_task_finished(taskInfo24, 1737422339870ms, 1737429539870ms);
			taskInfo24.newTask = false;

			p1_critical.state = TaskState::FINISHED;
			p1_critical.finishTime = 1737430439870ms;

			helper.required_messages({ &taskInfo24, &p1_critical });
		}

		SECTION("Building a Totally New Grouping")
		{
			helper.clear_message_output();

			configure.groupTasksBy.clear();
			configure.groupTasksBy.push_back("severity");
			configure.groupTasksBy.push_back("priority");

			helper.curl.requestResponse.clear();
			helper.curl.current = 0;

			helper.curl.requestResponse.emplace_back(fieldsResponse);

			helper.curl.requestResponse.emplace_back("{ \"bugs\": [ { \"id\": 50, \"summary\": \"bug 1\", \"status\": \"Assigned\", \"priority\": \"P2\", \"severity\": \"Minor\" },"
				"{ \"id\": 55, \"summary\": \"bug 2\", \"status\": \"Changes Made\", \"priority\": \"P2\", \"severity\": \"Minor\" },"
				"{ \"id\": 60, \"summary\": \"bug 3\", \"status\": \"Changes Made\", \"priority\": \"P1\", \"severity\": \"Critical\" },"
				"{ \"id\": 65, \"summary\": \"bug 4\", \"status\": \"Reviewed\", \"priority\": \"P3\", \"severity\": \"Blocker\" },"
				"{ \"id\": 70, \"summary\": \"bug 5\", \"status\": \"Confirmed\", \"priority\": \"P4\", \"severity\": \"Nitpick\" } ] }");

			helper.api.process_packet(configure, helper.output);

			auto nitpick = TaskInfoMessage(TaskID(27), TaskID(1), "Nitpick");
			auto minor = TaskInfoMessage(TaskID(32), TaskID(1), "Minor");
			auto critical = TaskInfoMessage(TaskID(37), TaskID(1), "Critical");
			auto blocker = TaskInfoMessage(TaskID(42), TaskID(1), "Blocker");

			auto nitpick_p1 = TaskInfoMessage(TaskID(28), TaskID(27), "P1");
			auto nitpick_p2 = TaskInfoMessage(TaskID(29), TaskID(27), "P2");
			auto nitpick_p3 = TaskInfoMessage(TaskID(30), TaskID(27), "P3");
			auto nitpick_p4 = TaskInfoMessage(TaskID(31), TaskID(27), "P4");

			auto minor_p1 = TaskInfoMessage(TaskID(33), TaskID(32), "P1");
			auto minor_p2 = TaskInfoMessage(TaskID(34), TaskID(32), "P2");
			auto minor_p3 = TaskInfoMessage(TaskID(35), TaskID(32), "P3");
			auto minor_p4 = TaskInfoMessage(TaskID(36), TaskID(32), "P4");

			auto critical_p1 = TaskInfoMessage(TaskID(38), TaskID(37), "P1");
			auto critical_p2 = TaskInfoMessage(TaskID(39), TaskID(37), "P2");
			auto critical_p3 = TaskInfoMessage(TaskID(40), TaskID(37), "P3");
			auto critical_p4 = TaskInfoMessage(TaskID(41), TaskID(37), "P4");

			auto blocker_p1 = TaskInfoMessage(TaskID(43), TaskID(42), "P1");
			auto blocker_p2 = TaskInfoMessage(TaskID(44), TaskID(42), "P2");
			auto blocker_p3 = TaskInfoMessage(TaskID(45), TaskID(42), "P3");
			auto blocker_p4 = TaskInfoMessage(TaskID(46), TaskID(42), "P4");

			setup_task_finished(nitpick, 1737427739870ms, 1737463739870ms);
			setup_task_finished(minor, 1737436739870ms, 1737472739870ms);
			setup_task_finished(critical, 1737445739870ms, 1737481739870ms);
			setup_task_finished(blocker, 1737454739870ms, 1737490739870ms);

			setup_task_finished(nitpick_p1, 1737429539870ms, 1737465539870ms);
			setup_task_finished(nitpick_p2, 1737431339870ms, 1737467339870ms);
			setup_task_finished(nitpick_p3, 1737433139870ms, 1737469139870ms);
			setup_task_finished(nitpick_p4, 1737434939870ms, 1737470939870ms);

			setup_task_finished(minor_p1, 1737438539870ms, 1737474539870ms);
			setup_task_finished(minor_p2, 1737440339870ms, 1737476339870ms);
			setup_task_finished(minor_p3, 1737442139870ms, 1737478139870ms);
			setup_task_finished(minor_p4, 1737443939870ms, 1737479939870ms);

			setup_task_finished(critical_p1, 1737447539870ms, 1737483539870ms);
			setup_task_finished(critical_p2, 1737449339870ms, 1737485339870ms);
			setup_task_finished(critical_p3, 1737451139870ms, 1737487139870ms);
			setup_task_finished(critical_p4, 1737452939870ms, 1737488939870ms);

			setup_task_finished(blocker_p1, 1737456539870ms, 1737492539870ms);
			setup_task_finished(blocker_p2, 1737458339870ms, 1737494339870ms);
			setup_task_finished(blocker_p3, 1737460139870ms, 1737496139870ms);
			setup_task_finished(blocker_p4, 1737461939870ms, 1737497939870ms);

			helper.required_messages(
				{
					&taskInfo4, &taskInfo5, &taskInfo8, &taskInfo11, &taskInfo14,

					&p1,
					&p1_critical,
					&p2,
					&p2_minor,
					&p3,
					&p3_blocker,
					&p4,
					&p4_nitpick,

					&nitpick,
					&nitpick_p1, &nitpick_p2, & nitpick_p3, &nitpick_p4,
					&minor,
					&minor_p1, &minor_p2, &minor_p3, &minor_p4,
					&critical,
					&critical_p1, &critical_p2, &critical_p3, &critical_p4,
					&blocker,
					&blocker_p1, &blocker_p2, &blocker_p3, &blocker_p4,

					
				});
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

		CHECK(helper.curl.requestResponse[0].request == "0.0.0.0/rest/bug?assigned_to=test&api_key=asfesdFEASfslj&resolution=---");

		auto taskInfo22 = TaskInfoMessage(TaskID(22), TaskID(9), "50 - bug 1");
		auto taskInfo23 = TaskInfoMessage(TaskID(23), TaskID(9), "55 - bug 2");
		auto taskInfo24 = TaskInfoMessage(TaskID(24), TaskID(5), "60 - bug 3");
		auto taskInfo25 = TaskInfoMessage(TaskID(25), TaskID(16), "65 - bug 4");
		auto taskInfo26 = TaskInfoMessage(TaskID(26), TaskID(18), "70 - bug 5");

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

		setup_task_inactive(taskInfo22, 1737418739870ms);
		setup_task_inactive(taskInfo23, 1737420539870ms);
		setup_task_inactive(taskInfo24, 1737422339870ms);
		setup_task_inactive(taskInfo25, 1737424139870ms);
		setup_task_inactive(taskInfo26, 1737425939870ms);

		setup_task_inactive(p1, 1737345839870ms);
		setup_task_inactive(p2, 1737354839870ms);
		setup_task_inactive(p3, 1737363839870ms);
		setup_task_inactive(p4, 1737372839870ms);
		setup_task_inactive(p1_critical, 1737351239870ms);
		setup_task_inactive(p2_minor, 1737358439870ms);
		setup_task_inactive(p3_blocker, 1737371039870ms);
		setup_task_inactive(p4_nitpick, 1737374639870ms);

		p1.newTask = p2.newTask = p3.newTask = p4.newTask = p1_critical.newTask = p2_minor.newTask = p3_blocker.newTask = p4_nitpick.newTask = false;
		p1.finishTime = p2.finishTime = p3.finishTime = p4.finishTime = p1_critical.finishTime = p2_minor.finishTime = p3_blocker.finishTime = p4_nitpick.finishTime = std::nullopt;

		helper.required_messages(
			{
				&taskInfo22, &taskInfo23, &taskInfo24, &taskInfo25, &taskInfo26,
				&p1, &p1_critical, &p2, &p2_minor, &p3, &p3_blocker, &p4, &p4_nitpick
			});
	}
}

