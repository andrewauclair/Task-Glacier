add_library (micro_task_common OBJECT
	clock.hpp
	packets.hpp
	packets.cpp
)

target_include_directories(micro_task_common PUBLIC .)

set_target_properties(micro_task_common PROPERTIES CXX_STANDARD 23)

target_link_libraries(micro_task_common PUBLIC
	strong_type
)
