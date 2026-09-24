cmake_minimum_required(VERSION 3.10)
project(http_server LANGUAGES CXX)

find_package(Boost REQUIRED COMPONENTS system)

add_subdirectory(src)