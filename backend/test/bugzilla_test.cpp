#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include "api.hpp"
#include "server.hpp"
#include "packets.hpp"
#include "utils.h"

#include <vector>
#include <source_location>

TEST_CASE("Configuring Bugzilla Information", "[bugzilla][api]")
{
	TestHelper<nullDatabase> helper;

	helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "Bugzilla"));
	helper.clear_message_output();

	// send bugzilla packet
	auto configure = BugzillaInfoMessage(BugzillaInstanceID(0), "bugzilla", "0.0.0.0", "asfesdFEASfslj");
	configure.username = "test";
	configure.rootTaskID = TaskID(1);
	configure.groupTasksBy.push_back("product");
	configure.groupTasksBy.push_back("severity");

	configure.labelToField["Priority"] = "priority";
	configure.labelToField["Status"] = "status";

	helper.curl.requestResponse.emplace_back("{ \"fields\": [] }");
	helper.curl.requestResponse.emplace_back("{ \"bugs\": [] }");

	helper.api.process_packet(configure, helper.output);
	configure.instanceID = BugzillaInstanceID(1);

	helper.required_messages({ &configure });
}

TEST_CASE("Request Bugzilla Information", "[bugzilla][api]")
{
	TestHelper<nullDatabase> helper;

	helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "Bugzilla"));
	helper.clear_message_output();

	// send bugzilla packet
	auto configure = BugzillaInfoMessage(BugzillaInstanceID(0), "bugzilla", "0.0.0.0", "asfesdFEASfslj");
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

	configure.instanceID = BugzillaInstanceID(1);

	auto bulk_start = BasicMessage(PacketType::BULK_TASK_INFO_START);
	auto bulk_finish = BasicMessage(PacketType::BULK_TASK_INFO_FINISH);

	helper.required_messages({ &timeCategories, &bulk_start, &root, &bulk_finish, &configure, &complete });
}

TEST_CASE("Configuring Multiple Bugzilla Instances", "[bugzilla][api]")
{
	TestHelper<nullDatabase> helper;

	helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "Bugzilla"));

	// send bugzilla packet
	auto configure = BugzillaInfoMessage(BugzillaInstanceID(0), "bugzilla", "0.0.0.0", "asfesdFEASfslj");
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

		configure.instanceID = BugzillaInstanceID(1);
		configure2.instanceID = BugzillaInstanceID(2);

		auto bulk_start = BasicMessage(PacketType::BULK_TASK_INFO_START);
		auto bulk_finish = BasicMessage(PacketType::BULK_TASK_INFO_FINISH);

		helper.required_messages({ &timeCategories, &bulk_start, &root, &bulk_finish, &configure, &configure2, &complete });
	}
}

TEST_CASE("Bugzilla Refresh", "[bugzilla][api]")
{
	using namespace std::chrono_literals;

	TestHelper<nullDatabase> helper;

	helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "Bugzilla"));

	// send bugzilla packet
	auto configure = BugzillaInfoMessage(BugzillaInstanceID(0), "bugzilla", "0.0.0.0", "asfesdFEASfslj");
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
	configure.instanceID = BugzillaInstanceID(1);

	CHECK(helper.curl.requestResponse[0].request == "0.0.0.0/rest/field/bug?api_key=asfesdFEASfslj");

	auto root = TaskInfoMessage(TaskID(1), NO_PARENT, "Bugzilla");
	root.createTime = 1737344039870ms;
	root.newTask = true;

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

		CHECK(helper.curl.requestResponse[0].request == "0.0.0.0/rest/bug?assigned_to=test&api_key=asfesdFEASfslj&last_change_time=2025-01-20T03:48:59Z");

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

		taskInfo5.indexInParent = 1;
		p1.indexInParent = 1;
		p3.indexInParent = 2;
		p4.indexInParent = 3;

		setup_task_inactive(taskInfo4, 1737348539870ms);
		setup_task_inactive(taskInfo5, 1737349439870ms);
		setup_task_inactive(taskInfo8, 1737352139870ms);
		setup_task_inactive(taskInfo11, 1737354839870ms);
		setup_task_inactive(taskInfo14, 1737357539870ms);

		setup_task_inactive(p1, 1737350339870ms);
		setup_task_inactive(p2, 1737346739870ms);
		setup_task_inactive(p3, 1737353039870ms);
		setup_task_inactive(p4, 1737355739870ms);
		setup_task_inactive(p1_critical, 1737351239870ms);
		setup_task_inactive(p2_minor, 1737347639870ms);
		setup_task_inactive(p3_blocker, 1737353939870ms);
		setup_task_inactive(p4_nitpick, 1737356639870ms);

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

			CHECK(helper.curl.requestResponse[0].request == "0.0.0.0/rest/bug?assigned_to=test&api_key=asfesdFEASfslj&last_change_time=2025-01-20T04:03:59Z");

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
			taskInfo27.indexInParent = 1;

			setup_task_inactive(taskInfo27, 1737359339870ms);

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
			taskInfo4.finishTime = 1737359339870ms;

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

				setup_task_inactive(p5, 1737359339870ms);
				setup_task_inactive(p5_nitpick, 1737360239870ms);
				setup_task_finished(p4, 1737355739870ms, 1737361139870ms);
				setup_task_finished(p4_nitpick, 1737356639870ms, 1737362039870ms);

				taskInfo14.newTask = false;
				taskInfo14.parentID = TaskID(16);

				p4_nitpick.newTask = false;
				p4.newTask = false;
				p4.indexInParent = 3;

				p5.indexInParent = 4;

				helper.required_messages({ &p5, &p5_nitpick, &taskInfo14, &p4, &p4_nitpick });
			}

			SECTION("Change Severity (Second Layer of Grouping)")
			{
				helper.clear_message_output();

				helper.curl.requestResponse.emplace_back("{ \"bugs\": [ { \"id\": 70, \"summary\": \"bug 5\", \"status\": \"Confirmed\", \"priority\": \"P4\", \"severity\": \"Minor2\" } ] }");

				helper.expect_success(refresh);

				auto p4_minor2 = TaskInfoMessage(TaskID(15), TaskID(12), "Minor2");

				setup_task_inactive(p4_minor2, 1737359339870ms);
				setup_task_finished(p4_nitpick, 1737356639870ms, 1737360239870ms);

				taskInfo14.newTask = false;
				taskInfo14.parentID = TaskID(15);

				p4_nitpick.newTask = false;
				
				p4_minor2.indexInParent = 1;

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

				setup_task_inactive(p5, 1737359339870ms);
				setup_task_inactive(p5_nitpick, 1737360239870ms);

				taskInfo14.newTask = false;
				taskInfo14.parentID = TaskID(16);

				p5.indexInParent = 4;

				auto taskInfo29 = TaskInfoMessage(TaskID(17), TaskID(16), "75 - bug 6");

				setup_task_inactive(taskInfo29, 1737361139870ms);

				helper.required_messages({ &p5, &p5_nitpick, &taskInfo29 });
			}

			SECTION("Change Severity (Second Layer of Grouping)")
			{
				helper.clear_message_output();

				helper.curl.requestResponse.emplace_back("{ \"bugs\": [ { \"id\": 75, \"summary\": \"bug 6\", \"status\": \"Confirmed\", \"priority\": \"P4\", \"severity\": \"Minor2\" } ] }");

				helper.expect_success(refresh);

				auto p4_minor2 = TaskInfoMessage(TaskID(15), TaskID(12), "Minor2");
				p4_minor2.indexInParent = 1;

				setup_task_inactive(p4_minor2, 1737359339870ms);

				taskInfo14.newTask = false;
				taskInfo14.parentID = TaskID(15);

				auto taskInfo28 = TaskInfoMessage(TaskID(16), TaskID(15), "75 - bug 6");

				setup_task_inactive(taskInfo28, 1737360239870ms);

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
			taskInfo24.createTime = 1737352139870ms;

			p1_critical.state = TaskState::FINISHED;
			p1_critical.finishTime = 1737375539870ms;
			
			auto p2_critical = TaskInfoMessage(TaskID(15), TaskID(2), "Critical");
			p2_critical.indexInParent = 1;
			p1.indexInParent = 1;

			setup_task_inactive(p2_critical, 1737359339870ms);

			setup_task_finished(p1, 1737350339870ms, 1737360239870ms);
			p1.newTask = false;
			p1_critical.newTask = false;
			p1_critical.finishTime = 1737361139870ms;

			helper.required_messages({ &p2_critical, &taskInfo24, &p1, &p1_critical });
		}

		SECTION("Finishing Old Grouping Tasks - Last Bug is Finished")
		{
			helper.clear_message_output();

			helper.curl.requestResponse.emplace_back("{ \"bugs\": [ { \"id\": 60, \"summary\": \"bug 3\", \"status\": \"RESOLVED\", \"priority\": \"P1\", \"severity\": \"Critical\" } ] }");

			helper.expect_success(refresh);

			auto taskInfo8 = TaskInfoMessage(TaskID(8), TaskID(7), "60 - bug 3");
			setup_task_finished(taskInfo8, 1737352139870ms, 1737359339870ms);
			taskInfo8.newTask = false;

			p1.state = TaskState::FINISHED;
			p1.finishTime = 1737360239870ms;
			p1.newTask = false;
			
			p1_critical.state = TaskState::FINISHED;
			p1_critical.finishTime = 1737361139870ms;
			p1_critical.newTask = false;

			helper.required_messages({ &taskInfo8, &p1, &p1_critical });
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

			auto nitpick = TaskInfoMessage(TaskID(21), TaskID(1), "Nitpick");
			auto minor = TaskInfoMessage(TaskID(15), TaskID(1), "Minor");
			auto critical = TaskInfoMessage(TaskID(17), TaskID(1), "Critical");
			auto blocker = TaskInfoMessage(TaskID(19), TaskID(1), "Blocker");

			auto nitpick_p4 = TaskInfoMessage(TaskID(22), TaskID(21), "P4");
			auto minor_p2 = TaskInfoMessage(TaskID(16), TaskID(15), "P2");
			auto critical_p1 = TaskInfoMessage(TaskID(18), TaskID(17), "P1");
			auto blocker_p3 = TaskInfoMessage(TaskID(20), TaskID(19), "P3");

			setup_task_inactive(nitpick, 1737364739870ms);
			setup_task_inactive(minor, 1737359339870ms);
			setup_task_inactive(critical, 1737361139870ms);
			setup_task_inactive(blocker, 1737362939870ms);

			setup_task_inactive(nitpick_p4, 1737365639870ms);
			setup_task_inactive(minor_p2, 1737360239870ms);
			setup_task_inactive(critical_p1, 1737362039870ms);
			setup_task_inactive(blocker_p3, 1737363839870ms);

			taskInfo4.parentID = TaskID(16);
			taskInfo5.parentID = TaskID(16);
			taskInfo8.parentID = TaskID(18);
			taskInfo11.parentID = TaskID(20);
			taskInfo14.parentID = TaskID(22);
			taskInfo4.newTask = taskInfo5.newTask = taskInfo8.newTask = taskInfo11.newTask = taskInfo14.newTask = false;
			
			setup_task_finished(p2, 1737346739870ms, 1737366539870ms);
			setup_task_finished(p1, 1737350339870ms, 1737368339870ms);
			setup_task_finished(p3, 1737353039870ms, 1737370139870ms);
			setup_task_finished(p4, 1737355739870ms, 1737371939870ms);
			p2.newTask = false;
			p1.newTask = false;
			p3.newTask = false;
			p4.newTask = false;
			
			setup_task_finished(p2_minor, 1737347639870ms, 1737367439870ms);
			setup_task_finished(p1_critical, 1737351239870ms, 1737369239870ms);
			setup_task_finished(p3_blocker, 1737353939870ms, 1737371039870ms);
			setup_task_finished(p4_nitpick, 1737356639870ms, 1737372839870ms);
			p2_minor.newTask = false;
			p1_critical.newTask = false;
			p3_blocker.newTask = false;
			p4_nitpick.newTask = false;

			minor.indexInParent = 4;
			taskInfo5.indexInParent = 1;
			critical.indexInParent = 5;
			blocker.indexInParent = 6;
			nitpick.indexInParent = 7;

			helper.required_messages(
				{
					&configure,

					&minor, &minor_p2, &taskInfo4, &taskInfo5,
					&critical, &critical_p1, &taskInfo8, 
					&blocker, &blocker_p3, &taskInfo11, 
					&nitpick, &nitpick_p4, &taskInfo14,
					
					&p2,
					&p2_minor,
					&p1,
					&p1_critical,
					&p3,
					&p3_blocker,
					&p4,
					&p4_nitpick
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

		CHECK(helper.curl.requestResponse[0].request == "0.0.0.0/rest/bug?assigned_to=test&api_key=asfesdFEASfslj&last_change_time=2025-01-20T03:48:59Z");

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

		setup_task_inactive(taskInfo4, 1737348539870ms);
		setup_task_inactive(taskInfo5, 1737349439870ms);
		setup_task_inactive(taskInfo8, 1737352139870ms);
		setup_task_inactive(taskInfo11, 1737354839870ms);
		setup_task_inactive(taskInfo14, 1737357539870ms);

		setup_task_inactive(p1, 1737350339870ms);
		setup_task_inactive(p2, 1737346739870ms);
		setup_task_inactive(p3, 1737353039870ms);
		setup_task_inactive(p4, 1737355739870ms);
		setup_task_inactive(p1_critical, 1737351239870ms);
		setup_task_inactive(p2_minor, 1737347639870ms);
		setup_task_inactive(p3_blocker, 1737353939870ms);
		setup_task_inactive(p4_nitpick, 1737356639870ms);

		taskInfo5.indexInParent = 1;
		p1.indexInParent = 1;
		p3.indexInParent = 2;
		p4.indexInParent = 3;

		p1.newTask = p2.newTask = p3.newTask = p4.newTask = p1_critical.newTask = p2_minor.newTask = p3_blocker.newTask = p4_nitpick.newTask = true;

		helper.required_messages(
			{
				&p2, &p2_minor, &taskInfo4, &taskInfo5, 
				&p1, &p1_critical, &taskInfo8, 
				&p3, &p3_blocker, &taskInfo11, 
				&p4, &p4_nitpick, &taskInfo14
			});
	}
}

