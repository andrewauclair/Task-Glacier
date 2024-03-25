add_library (micro_task_lib STATIC 
	src/lib.cpp 
	src/packets.cpp
	include/lib.hpp
	include/packets.hpp
)

target_include_directories(micro_task_lib PUBLIC include)

set_target_properties(micro_task_lib PROPERTIES CXX_STANDARD 23)

target_link_libraries(micro_task_lib PUBLIC
	strong_type
)
