cmake_minimum_required(VERSION 3.5)

project(scope LANGUAGES CXX)

find_package(Qt5 REQUIRED COMPONENTS Widgets)

add_executable(scope scope.cc)
include_directories(../lib ../../artix7_v1/fw/modules/usbadc-support/sw/)

target_link_directories(scope PRIVATE ../lib/bld ../../artix7_v1/fw/modules/usbadc-support/sw/)
target_link_libraries(scope PRIVATE Qt5::Widgets fwLib fwcomm)


