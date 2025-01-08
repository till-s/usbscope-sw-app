cmake_minimum_required(VERSION 3.5)

project(scope LANGUAGES CXX)

find_package(Qt5 REQUIRED COMPONENTS Widgets)

find_package(HDF5 COMPONENTS C)

find_library(QWT NAMES qwt qwt6 qwt-qt5)
set(AUTOMOC ON)

set(SRCS
	"scope.cc"
	"SysPipe.cpp"
	"ScopeReader.cpp"
	"MovableMarkers.cpp"
)

set(LIBS ${QWT} Qt5::Widgets ${FWLIB} ${FWCOMM})
if (HDF5_FOUND)
	list(APPEND LIBS ${HDF5_LIBRARIES})
endif()

add_executable(scope ${SRCS})
include_directories(./ ../lib ../../artix7_v1/fw/modules/usbadc-support/sw/ /usr/include/qwt)
set_target_properties(scope PROPERTIES AUTOMOC ON)

find_library(FWLIB fwLib ../lib/bld)
find_library(FWCOMM fwcomm ../../artix7_v1/fw/modules/usbadc-support/sw/)
target_link_libraries(scope PRIVATE ${LIBS})
