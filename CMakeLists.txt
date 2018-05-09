cmake_minimum_required(VERSION 3.8)
project(natTraverseClientMuduo)

add_definitions("-Wall -g")
set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp ./stun/stun.cxx ./stun/udp.cxx ./stun/client.cxx stun/client.hpp udpTraverse/udpTraverse.cpp udpTraverse/udpTraverse.hpp udpTurn/udpTurn.cpp udpTurn/udpTurn.hpp stun/MyStun.cpp stun/MyStun.hpp heartBeat/heartBeat.cpp heartBeat/heartBeat.hpp main.hpp)



include_directories(./release-install/include)
link_directories("./release-install/lib")
link_libraries(muduo_base muduo_net  pthread json)
execute_process(COMMAND sshpass -p tl scp natTraverseClientMuduo tl@218.192.170.178:/home/tl/project/test)

add_executable(natTraverseClientMuduo ${SOURCE_FILES})