#pragma once

#include "clock.hpp"
#include "packets.hpp"
#include "curl.hpp"

#include <simdjson.h>

#include <chrono>
#include <string>
#include <map>
#include <optional>

class MicroTask;
class API;

struct BugzillaInstance
{
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
};

class Bugzilla
{
public:
	Bugzilla(const Clock& clock, cURL& curl)
		: m_clock(&clock),
		m_curl(&curl)
	{
	}

	void receive_info(const BugzillaInfoMessage& info, MicroTask& app, API& api, std::vector<std::unique_ptr<Message>>& output, std::ostream& file);
	void send_info(std::vector<std::unique_ptr<Message>>& output);

	void refresh(const RequestMessage& request, MicroTask& app, API& api, std::ostream& file, std::vector<std::unique_ptr<Message>>& output);

	void load_config(const std::string& line, std::istream& input);
	void load_refresh(const std::string& line);

private:
	void build_group_by_task(BugzillaInstance& instance, MicroTask& app, API& api, std::vector<std::unique_ptr<Message>>& output, TaskID parent, std::span<const std::string> groupTaskBy);

	class Task* parent_task_for_bug(BugzillaInstance& instance, MicroTask& app, API& api, std::vector<std::unique_ptr<Message>>& output, const simdjson::dom::element& bug, TaskID currentParent, std::span<const std::string> groupTaskBy);

	const Clock* m_clock;
	cURL* m_curl;

	std::map<std::string, BugzillaInstance> m_bugzilla;
	std::optional<std::chrono::milliseconds> m_lastBugzillaRefresh;
};
