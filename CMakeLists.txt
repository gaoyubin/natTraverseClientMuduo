cmake_minimum_required(VERSION 3.8)
#project(natTraverseClientMuduo)
project(natTraverse)

add_definitions("-Wall -g")
set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES natTraverse.cpp ./stun/stun.cxx ./stun/udp.cxx ./stun/client.cxx stun/client.hpp udpTraverse/udpTraverse.cpp udpTraverse/udpTraverse.hpp udpTurn/udpTurn.cpp udpTurn/udpTurn.hpp stun/MyStun.cpp stun/MyStun.hpp heartBeat/heartBeat.cpp heartBeat/heartBeat.hpp natTraverse.hpp tcpTurn/tcpTurn.cpp tcpTurn/tcpTurn.hpp)



set(CMAKE_EXE_LINKER_FLAGS  "-static")

include_directories(./release-install/include)
link_directories("./release-install/lib")
link_libraries(muduo_base muduo_net  pthread json)
#execute_process(COMMAND sshpass -p tl scp natTraverseClientMuduo tl@218.192.170.178:/home/tl/project/test)
add_executable(natTraverseClientMuduo ${SOURCE_FILES})

SET(CMAKE_INSTALL_PREFIX ${PROJECT_BINARY_DIR})

#message(${natTraverseClientMuduo_SOURCE_DIR})
#message(${PROJECT_SOURCE_DIR})
#message(${PROJECT_BINARY_DIR})
#message(${CMAKE_INSTALL_PREFIX})
#message("hello world")


#add_library(natTraverse ${SOURCE_FILES})
#install(FILES ${PROJECT_SOURCE_DIR}/natTraverse.hpp DESTINATION natTraverse)
#install (TARGETS natTraverse ARCHIVE DESTINATION natTraverse)
#add_custom_command(
#        TARGET natTraverse
#        #Name ALL
#        COMMAND ./install_copy.sh

#)