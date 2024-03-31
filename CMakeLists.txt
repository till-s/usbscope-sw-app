cmake_minimum_required(VERSION 3.5)

project(scope LANGUAGES CXX)

find_package(Qt5 REQUIRED COMPONENTS Widgets)

find_library(QWT NAMES qwt qwt6 qwt-qt5)
set(AUTOMOC ON)

add_executable(scope scope.cc SysPipe.cpp ScopeReader.cpp MovableMarkers.cpp)
include_directories(./ ../lib ../../artix7_v1/fw/modules/usbadc-support/sw/ /usr/include/qwt)
set_target_properties(scope PROPERTIES AUTOMOC ON)

target_link_directories(scope PRIVATE ../lib/bld ../../artix7_v1/fw/modules/usbadc-support/sw/)
target_link_libraries(scope PRIVATE ${QWT} Qt5::Widgets fwLib fwcomm)


