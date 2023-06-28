cmake_minimum_required(VERSION 3.16.3)

project(simple_http_dos_protection)

file(GLOB SOURCES
    *.h
    *.cpp
)

add_executable ( dos_clients ${SOURCES} )
