cmake_minimum_required(VERSION 3.22.1)
project(Bookstore-2024)

set(CMAKE_CXX_STANDARD 20)

add_compile_options(-Wall)
add_compile_options(-Ofast)

add_executable(code 
    src/Initialization.hpp
    src/MemoryRiver.hpp
    src/Database.hpp
    src/User.hpp
    src/CommandParser.hpp
    src/AsyncLogger.hpp
    code.cpp
)