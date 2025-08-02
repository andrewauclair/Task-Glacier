#ifndef MICRO_TASK_PACKETS_HPP
#define MICRO_TASK_PACKETS_HPP

#include <vector>
#include <variant>
#include <string>
#include <cstring>
#include <memory>
#include <expected>
#include <span>
#include <optional>
#include <chrono>
#include <map>
#include <array>
#include <cassert>

#include <strong_type/strong_type.hpp>

#include "packets/basic.hpp"
#include "packets/bugzilla_info.hpp"
#include "packets/bugzilla_instance_id.hpp"
#include "packets/create_task.hpp"
#include "packets/daily_report.hpp"
#include "packets/failure_response.hpp"
#include "packets/message.hpp"
#include "packets/packet_builder.hpp"
#include "packets/packet_parser.hpp"
#include "packets/request.hpp"
#include "packets/request_daily_report.hpp"
#include "packets/request_id.hpp"
#include "packets/request_weekly_report.hpp"
#include "packets/success_response.hpp"
#include "packets/task.hpp"
#include "packets/task_id.hpp"
#include "packets/task_info.hpp"
#include "packets/task_state.hpp"
#include "packets/task_times.hpp"
#include "packets/time_category.hpp"
#include "packets/time_category_id.hpp"
#include "packets/time_category_mod_type.hpp"
#include "packets/time_code.hpp"
#include "packets/time_code_id.hpp"
#include "packets/time_entry.hpp"
#include "packets/time_entry_data_packet.hpp"
#include "packets/time_entry_modify_packet.hpp"
#include "packets/unpack_error.hpp"
#include "packets/update_task.hpp"
#include "packets/weekly_report.hpp"






























struct SearchRequestMessage : Message
{
	// search text
	// type: task name, label, time, state (mainly for finished), time category
	std::string searchText;

	// ideally this would work kind of like the IntelliJ search anywhere, but with some special stuff for time
	// or maybe certain text is detected as time like "Jan 29" or "January 29" or "1/29"?
};

struct SearchResultMessage : Message
{
	std::vector<TaskID> taskIDs;
};

// time categories
// these will be tracked internally by ID so that we don't have to pass strings everywhere
// and the user can freely rename them and have all uses update
// the time categories can be removed after adding, but only if they have not been used
// the user can optionally reassign any times to a new time category continue removing
//
// time categories can be archived when in use. this will remove them from being an option
// for creating new tasks, but will allow them to still exist on old tasks. Some option might
// be needed for existing tasks that are using the time category. Presumably, if you are archiving
// a time category, it's no longer in use. so in this case, you should probably change any
// active tasks to another time category.






#endif
