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
#include "packets/version.hpp"

#endif
