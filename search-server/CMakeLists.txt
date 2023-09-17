cmake_minimum_required(VERSION 3.11)

project(Search_server CXX)
set(CMAKE_CXX_STANDARD 17)

if(CMAKE_SYSTEM_NAME MATCHES "^MINGW")
   set(SYSTEM_LIBS -lstdc++)
else()
   set(SYSTEM_LIBS)
endif()

set(TEST_FILES log_duration.h
    test_example_functions.h test_example_functions.cpp)

set(SERVER_HEADERS document.h
		   paginator.h
		   read_input_functions.h
		   request_queue.h
		   search_server.h
		   remove_duplicates.h
		   string_processing.h
		   concurrent_map.h)

add_executable(search_server ${TEST_FILES}  ${SERVER_HEADERS} main.cpp  document.cpp 
		read_input_functions.cpp 
		request_queue.cpp 
		search_server.cpp 
		remove_duplicates.cpp 
		string_processing.cpp)
	