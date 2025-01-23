add_library (task-glacier-server-lib STATIC 
	api.hpp		api.cpp
	server.cpp  server.hpp
	persist.hpp persist.cpp
)

target_include_directories(task-glacier-server-lib PUBLIC .)

set_target_properties(task-glacier-server-lib PROPERTIES CXX_STANDARD 23)

target_link_libraries(task-glacier-server-lib PUBLIC
	strong_type task-glacier-common
)
