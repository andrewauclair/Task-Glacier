#pragma once

#include "clock.hpp"
#include "curl.hpp"

#include "packets/bugzilla_info.hpp"
#include "packets/bugzilla_instance_id.hpp"
#include "packets/task_id.hpp"
#include "packets/request.hpp"

#include <simdjson.h>

#include <chrono>
#include <string>
#include <map>
#include <optional>

#include "packet_sender.hpp"

class MicroTask;
class API;
class Database;

struct BugzillaInstance
{
	BugzillaInstanceID instanceID;
	std::string bugzillaName;
	std::string bugzillaURL;
	std::string bugzillaApiKey;
	std::string bugzillaUsername;
	TaskID bugzillaRootTaskID = NO_PARENT;
	std::vector<std::string> bugzillaGroupTasksBy;
	std::map<std::string, std::string> bugzillaLabelToField;
	std::optional<std::chrono::milliseconds> lastBugzillaRefresh;

	std::map<int, TaskID> bugToTaskID;

	std::map<std::string, std::vector<std::string>> fields;

	BugzillaInstance(BugzillaInstanceID instanceID) : instanceID(instanceID) {}
};

class Bugzilla
{
public:
	Bugzilla(const Clock& clock, cURL& curl, PacketSender& sender)
		: m_clock(&clock),
		m_curl(&curl),
		m_sender(&sender)
	{
	}

	void receive_info(const BugzillaInfoMessage& info, MicroTask& app, API& api, Database& database);
	void send_info();

	void refresh(const RequestMessage& request, MicroTask& app, API& api, Database& database);

	void load_instance(const BugzillaInstance& instance);
	void next_instance_id(BugzillaInstanceID next);

private:
	void build_group_by_task(BugzillaInstance& instance, MicroTask& app, API& api, TaskID parent, std::span<const std::string> groupTaskBy);

	class Task* parent_task_for_bug(BugzillaInstance& instance, MicroTask& app, API& api, const simdjson::dom::element& bug, TaskID currentParent, std::span<const std::string> groupTaskBy);

	const Clock* m_clock;
	cURL* m_curl;
	PacketSender* m_sender;

	BugzillaInstanceID m_nextBugzillaID = BugzillaInstanceID(1);
	std::map<std::string, BugzillaInstance> m_bugzilla;
};
