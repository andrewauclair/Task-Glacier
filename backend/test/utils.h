#pragma once

#include "curl.hpp"
#include "database.hpp"
#include "clock.hpp"
#include "api.hpp"
#include "test_clock.hpp"

#include <source_location>
#include <memory>

#include "packet_sender.hpp"
#include "message_compare.h"

struct curlRequestResponse
{
	std::string result;
	std::string request;
};

struct curlTest : cURL
{
	std::vector<curlRequestResponse> requestResponse;
	std::size_t current = 0;

	std::vector<std::string> requests;

	void clear()
	{
		requestResponse.clear();
		current = 0;
		requests.clear();
	}

	std::optional<std::string> execute_request(const std::string& request) override
	{
		requests.push_back(request);

		if (current >= requestResponse.size())
		{
			for (const std::string& request : requests)
			{
				UNSCOPED_INFO(request);
			}
			FAIL("More cURL requests than expected");
		}

		requestResponse[current].request = request;
		
		auto result = requestResponse[current].result;
		
		++current;

		return result;
	}
};

struct nullDatabase : Database
{
	void load(Bugzilla& bugzilla, MicroTask& app, API& api) override {}

	// write task
	void write_task(const Task& task, PacketSender& sender) override {}
	void write_next_task_id(TaskID nextID, PacketSender& sender) override {}

	// write bugzilla config
	void write_bugzilla_instance(const BugzillaInstance& instance, PacketSender& sender) override {}
	void write_next_bugzilla_instance_id(BugzillaInstanceID nextID, PacketSender& sender) override {}
	void remove_bugzilla_instance(int ID) override {}
	void bugzilla_refreshed(int ID) override {}

	// write time entry configuration
	// write sessions
	void write_session(TaskID task, const TaskTimes& session, PacketSender& sender) override {}
	void remove_sessions(TaskID task, PacketSender& sender) override {}

	// write time entries
	void write_time_entry(TaskID task, PacketSender& sender) override {}
	void remove_time_entry() override {}

	void write_time_entry_config(const TimeCategory& entry, PacketSender& sender) override {}
	void write_next_time_category_id(TimeCategoryID nextID, PacketSender& sender) override {}
	void write_next_time_code_id(TimeCodeID nextID, PacketSender& sender) override {}

	void remove_time_category(const TimeCategory& entry, PacketSender& sender) override {}
	void remove_time_code(const TimeCategory& entry, const TimeCode& code, PacketSender& sender) override {}

	void start_transaction(PacketSender& sender) override {}
	void finish_transaction(PacketSender& sender) override {}

	bool transaction_in_progress() const override { return false; }
};

struct TestPacketSender : PacketSender
{
	std::vector<std::unique_ptr<Message>> output;

	void send(std::unique_ptr<Message> message) override
	{
		output.emplace_back(std::move(message));
	}
};

template<typename DatabaseType>
struct TestHelper
{
	DatabaseType database;

	TestClock clock;
	curlTest curl;
	TestPacketSender sender;

	API api = API(clock, curl, database, sender);

	RequestID next_request_id()
	{
		auto id = m_next_request_id++;
		m_prev_request_id = id;
		return id;
	}

	RequestID prev_request_id() const
	{
		return m_prev_request_id;
	}

	void clear_message_output()
	{
		sender.output.clear();
	}

	void expect_success(const RequestMessage& message, std::source_location location = std::source_location::current())
	{
		sender.output.clear();

		api.process_packet(message);

		// check output for a success message for the request ID
		if (!sender.output.empty())
		{
			INFO("First message sent should be the response");

			INFO("");
			INFO(location.file_name() << ":" << location.line());

			verify_message(SuccessResponse(message.requestID), *sender.output[0]);

			// now remove the first message, calls to required_messages will check what comes after the response message
			sender.output.erase(sender.output.begin());
		}
		else
		{
			INFO("");
			INFO(location.file_name() << ":" << location.line());

			FAIL("Expected output message of SuccessResponse with request ID " << message.requestID._val);
		}
	}

	void expect_failure(const RequestMessage& message, const std::string& error, std::source_location location = std::source_location::current())
	{
		sender.output.clear();

		api.process_packet(message);

		// check output for a failure message for the request ID
		if (!sender.output.empty())
		{
			INFO("Only message sent should be the failure response");

			INFO("");
			INFO(location.file_name() << ":" << location.line());

			verify_message(FailureResponse(message.requestID, error), *sender.output[0]);

			REQUIRE(sender.output.size() == 1);
		}
		else
		{
			INFO("");
			INFO(location.file_name() << ":" << location.line());

			FAIL("Expected output message of FailureResponse with request ID " << message.requestID._val << " and error: " << error);
		}
	}

	void required_messages(const std::vector<Message*>& messages, std::source_location location = std::source_location::current())
	{
		// expected message sequence:
		// m1
		// m2
		// m3
		//
		// but found:
		// m2
		// m3

		bool match = sender.output.size() == messages.size();

		if (!match)
		{
			UNSCOPED_INFO("Expected " << messages.size() << " messages, but found " << sender.output.size());
		}

		for (std::size_t i = 0; i < sender.output.size() && match; i++)
		{
			verify_message(*messages[i], *sender.output[i]);
			/*if (*sender.output[i] != *messages[i])
			{
				UNSCOPED_INFO("Message " << (i + 1) << ". did not match expected message");
			}
			match &= *sender.output[i] == *messages[i];*/
		}
		/*for (std::size_t i = 0; i < sender.output.size() && match; i++)
		{
			if (*sender.output[i] != *messages[i])
			{
				UNSCOPED_INFO("Message " << (i + 1) << ". did not match expected message");
			}
			match &= *sender.output[i] == *messages[i];
		}*/

		if (!match)
		{
			UNSCOPED_INFO("Expected message sequence:");
			UNSCOPED_INFO("");

			int count = 1;

			for (auto&& msg : messages)
			{
				UNSCOPED_INFO(count << ". " << *msg);
				count++;
			}

			UNSCOPED_INFO("");
			UNSCOPED_INFO("Found message sequence:");
			UNSCOPED_INFO("");

			count = 1;

			for (auto&& msg : sender.output)
			{
				UNSCOPED_INFO(count << ". " << *msg);
				count++;
			}
		}

		INFO("");
		INFO(location.file_name() << ":" << location.line());

		CHECK(match);
	}

	RequestID m_next_request_id = RequestID(1);
	RequestID m_prev_request_id = RequestID(1);
};
