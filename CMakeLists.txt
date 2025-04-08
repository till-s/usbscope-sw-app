cmake_minimum_required(VERSION 3.5)

project(scope LANGUAGES CXX)

find_package(Qt5 REQUIRED COMPONENTS Widgets)

find_package(HDF5 COMPONENTS C)

add_subdirectory(usbadc-support/sw fwcomm)
add_subdirectory(fwcommCPP)

find_library(QWT NAMES qwt qwt6 qwt-qt5 fftw3)

set(AUTOMOC ON)

set(SRCS
	"scope.cc"
	"SysPipe.cpp"
	"ScopeReader.cpp"
	"MovableMarkers.cpp"
	"ScopeZoomer.cpp"
	"ScopePlot.cpp"
	"ScaleXfrm.cpp"
	"MessageDialog.cpp"
	"MenuButton.cpp"
	"TglButton.cpp"
	"ParamValidator.cpp"
	"MeasMarker.cpp"
	"Measurement.cpp"
)

set(LIBS ${QWT} Qt5::Widgets fwLib fwcomm fftw3)
if (HDF5_FOUND)
	list(APPEND LIBS ${HDF5_LIBRARIES})
endif()

include_directories(./ fwcommCPP usbadc-support/sw /usr/include/qwt)

add_executable(scope ${SRCS})
add_executable(h5t h5t.cc)
set_target_properties(scope PROPERTIES AUTOMOC ON)

target_link_libraries(scope PRIVATE ${LIBS})
target_link_libraries(h5t PRIVATE ${LIBS})
