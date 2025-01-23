add_library (task-glacier-common OBJECT
	clock.hpp
	packets.hpp
	packets.cpp
)

target_include_directories(task-glacier-common PUBLIC .)

set_target_properties(task-glacier-common PROPERTIES CXX_STANDARD 23)

target_link_libraries(task-glacier-common PUBLIC
	strong_type
)
