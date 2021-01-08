cmake_minimum_required(VERSION 3.16)
project(sqdbg)

include_directories(
        /usr/local/include
        /usr/include
)

link_directories(
        /usr/local/lib
        /usr/lib
)

link_libraries(libsqstdlib.so libsquirrel.so)

add_library(sqdbg serialize_state.inl sqrdbg.h sqrdbg.cpp sqdbgserver.h sqdbgserver.cpp)
add_executable(sqdb serialize_state.inl sqrdbg.h sqrdbg.cpp sqdbgserver.h sqdbgserver.cpp sqdb.cpp)