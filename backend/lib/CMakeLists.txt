set(PACKET_SOURCES

	packets/basic.hpp
	packets/bugzilla_info.hpp
	packets/bugzilla_instance_id.hpp
	packets/create_task.hpp
	packets/daily_report.hpp
	packets/error.hpp
	packets/failure_response.hpp
	packets/message.hpp
	packets/packet_builder.hpp
	packets/packet_parser.hpp
	packets/request.hpp
	packets/request_daily_report.hpp
	packets/request_id.hpp
	packets/request_weekly_report.hpp
	packets/success_response.hpp
	packets/task.hpp
	packets/task_id.hpp
	packets/task_info.hpp
	packets/task_state.hpp
	packets/task_state_change.hpp
	packets/task_times.hpp
	packets/time_category.hpp
	packets/time_category_id.hpp
	packets/time_category_mod_type.hpp
	packets/time_code.hpp
	packets/time_code_id.hpp
	packets/time_entry.hpp
	packets/time_entry_modify_packet.hpp
	packets/unpack_error.hpp
	packets/update_task.hpp
	packets/update_task_times.hpp
	packets/weekly_report.hpp
	packets/version.hpp
)
set(LIB_SOURCES

	api.hpp		api.cpp
	clock.hpp
	curl.hpp
	database.hpp database.cpp
	packets.hpp packets.cpp
	server.cpp  server.hpp
	bugzilla.cpp bugzilla.hpp
	packet_sender.hpp packet_sender.cpp
)

add_library (task-glacier-server-lib STATIC 
	${LIB_SOURCES}
	${PACKET_SOURCES}
)

source_group("Header Files/packets" FILES ${PACKET_SOURCES})

target_include_directories(task-glacier-server-lib PUBLIC .)

set_target_properties(task-glacier-server-lib PROPERTIES CXX_STANDARD 23)

target_link_libraries(task-glacier-server-lib PUBLIC
	strong_type simdjson SQLiteCpp
)
