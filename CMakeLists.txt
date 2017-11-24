project(sserv)
cmake_minimum_required(VERSION 2.6)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(LIB_INSTALL_PATH /usr/lib/)
set(LIB_INSTALL_PATH /usr/lib/mysql)
set(LIB_INSTALL_PATH /usr/local/lib/)

# Select flags.
SET(CMAKE_CXX_FLAGS "-Wall -fno-rtti")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")
SET(CMAKE_CXX_FLAGS_RELEASE "-O2")
SET(CMAKE_CXX_FLAGS_DEBUG  "-O0 -g")


INCLUDE_DIRECTORIES(/usr/include/mysql)
set(client_SRCS new_betta.cpp json_parser.cpp)
add_executable(sserv ${client_SRCS})
target_link_libraries(sserv mysqlclient yajl)
