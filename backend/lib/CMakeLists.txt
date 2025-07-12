add_library (task-glacier-server-lib STATIC 
	api.hpp		api.cpp
	clock.hpp
	curl.hpp
	database.hpp database.cpp
	packets.hpp packets.cpp
	persist.hpp persist.cpp
	server.cpp  server.hpp
	bugzilla.cpp bugzilla.hpp
)

target_include_directories(task-glacier-server-lib PUBLIC .)

set_target_properties(task-glacier-server-lib PROPERTIES CXX_STANDARD 23)

target_link_libraries(task-glacier-server-lib PUBLIC
	strong_type simdjson SQLiteCpp
)
