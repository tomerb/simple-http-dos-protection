cmake_minimum_required(VERSION 3.16.3)

project(simple_http_dos_protection)

file(GLOB SOURCES
    *.h
    *.cpp
)

add_executable ( dos_clients dos_clients.cpp dos_client.h dos_client.cpp )
add_executable ( dos_server dos_server_main.cpp dos_server.h dos_server.cpp )
