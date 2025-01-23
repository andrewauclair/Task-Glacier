add_library (task-glacier-server STATIC 
	api.hpp		api.cpp
	server.cpp  server.hpp
	persist.hpp persist.cpp
)

target_include_directories(task-glacier-server PUBLIC .)

set_target_properties(task-glacier-server PROPERTIES CXX_STANDARD 23)

target_link_libraries(task-glacier-server PUBLIC
	strong_type task-glacier-common
)
