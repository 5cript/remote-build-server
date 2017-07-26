# Version check
cmake_minimum_required (VERSION 3.0)

# Project
project(remote-build-server)

# Add files
file(GLOB sources "./*.cpp")

add_executable(remote-build-server ${sources})

# Search Directories
include_directories("..")

# Libraries
find_library(LATTENDER NAMES attender PATHS "../attender/build" "attender/build" STATIC)
find_library(LTINYPROC NAMES tiny-process-library PATHS "../tiny-process-library/build" "tiny-process-library/build" STATIC)
find_library(LSIMPLEJSON NAMES SimpleJSON PATHS "../SimpleJSON/build" "SimpleJSON/build" STATIC)
find_library(LBOOST_SYSTEM boost_system)
find_library(LBOOST_FILE_SYSTEM boost_filesystem)
find_library(LATOMIC atomic)
find_library(LSSL ssl)
find_library(LCRYPTO crypto)
find_library(LPTHREAD pthread)

if(LATOMIC STREQUAL "LATOMIC-NOTFOUND")
	set(LATOMIC "")
endif()
if(LPTHREAD STREQUAL "LPTHREAD-NOTFOUND")
	set(LPTHREAD "")
endif()
if(LBOOST_SYSTEM STREQUAL "LBOOST_SYSTEM-NOTFOUND")
	#message(FATAL_ERROR "Atomic library not found")
	find_library(LBOOST_SYSTEM boost_system-mt)
endif()
if(LBOOST_FILE_SYSTEM STREQUAL "LBOOST_FILE_SYSTEM-NOTFOUND")
	#message(FATAL_ERROR "Atomic library not found")
	find_library(LBOOST_FILE_SYSTEM boost_filesystem-mt)
endif()

set(LIBS "${LTINYPROC} ${LATTENDER} ${LSIMPLEJSON} ${LBOOST_SYSTEM} ${LBOOST_FILE_SYSTEM} ${LSSL} ${LCRYPTO}")

message("-- External libraries")
message("	${LATTENDER}")
message("	${LTINYPROC}")
message("	${LSIMPLEJSON}")

# MS SOCK
if (WIN32)
	find_library(LWS2_32 ws2_32)
	find_library(LMSWSOCK mswsock)
	
	set(LIBS "${LIBS} ${LWS2_32} ${LMSWSOCK}")
	
	message("-- Windows libraries are getting linked")
	message("	${LWS2_32}")
	message("	${LMSWSOCK}")
	
	if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		target_compile_definitions(remote-build-server PRIVATE BOOST_USE_WINDOWS_H=1)
	endif()
else()
	set(LWS2_32 "")
	set(LMSWSOCK "")
endif()

#add_library(attender STATIC IMPORTED GLOBAL)
#add_library(tiny-process-library STATIC IMPORTED)

#set_target_properties(attender PROPERTIES IMPORTED_LOCATION "../attender/build")
#set_target_properties(tiny-process-library PROPERTIES IMPORTED_LOCATION "../tiny-process-library/build")

target_compile_options(remote-build-server PRIVATE -fexceptions -std=c++14 -O3 -Wall -pedantic-errors -pedantic -static)
target_link_libraries(remote-build-server PRIVATE ${LTINYPROC} ${LATTENDER} ${LSIMPLEJSON} ${LBOOST_SYSTEM} ${LBOOST_FILE_SYSTEM} ${LSSL} ${LCRYPTO} ${LWS2_32} ${LMSWSOCK} atomic ${LPTHREAD})