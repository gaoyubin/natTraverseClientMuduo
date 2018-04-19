cmake_minimum_required(VERSION 3.8)
project(natTraverseClientMuduo)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp ./stun/stun.cxx ./stun/udp.cxx ./stun/client.cxx stun/client.hpp udpTraverse/udpTraverse.cpp udpTraverse/udpTraverse.hpp)

include_directories(./release-install/include)
link_directories("./release-install/lib")
link_libraries(muduo_base muduo_net  pthread json)
add_executable(natTraverseClientMuduo ${SOURCE_FILES})