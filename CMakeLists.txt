cmake_minimum_required(VERSION 3.20)
project(rk_sqllite)

set(CMAKE_CXX_STANDARD 17)

add_executable(rk_sqllite main.cpp MetaCommandResult.h Cursor.h Table.h Row.h Node.h Pager.cc Pager.h)
