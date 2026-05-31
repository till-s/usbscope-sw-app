#LB-MIT
#
# MIT License
#
# Copyright (c) 2026 Till Straumann
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
#LE-MIT

cmake_minimum_required(VERSION 3.20)

project(scope LANGUAGES CXX)

option(USE_QT6 "Use Qt6 - most likely you need to built QWT yourself!" OFF)
set(CACHE{QWT_QT6_PATH} TYPE PATH HELP "Path to QWT built against QT6 with lib/ and include/ subdirs" VALUE not-set-use-D)

if (USE_QT6)
	find_package(Qt6 REQUIRED COMPONENTS Widgets)
	set(QT_LIBS Qt6::Widgets)
	find_library(QWT NAMES qwt-qt6 PATHS ${QWT_QT6_PATH}/lib NO_DEFAULT_PATH)
	set(QWT_INC_PATH ${QWT_QT6_PATH}/include)
else()
	find_package(Qt5 REQUIRED COMPONENTS Widgets)
	set(QT_LIBS Qt5::Widgets)
	find_library(QWT NAMES qwt qwt6 qwt-qt5)
	set(QWT_INC_PATH /usr/include/qwt)
endif()

find_package(HDF5 COMPONENTS C)

add_subdirectory(usbadc-support/sw)
add_subdirectory(fwcommCPP)

find_library(FFTW3 NAMES fftw3)
find_library(JANSSON NAMES jansson)

set(AUTOMOC ON)

set(SRCS
	"scope.cc"
	"SysPipe.cpp"
	"ScopeReader.cpp"
	"MovableMarkers.cpp"
	"TrigCtrl.cpp"
	"ScopeZoomer.cpp"
	"ScopePlot.cpp"
	"ScaleXfrm.cpp"
	"MessageDialog.cpp"
	"MenuButton.cpp"
	"TglButton.cpp"
	"ParamValidator.cpp"
	"MeasMarker.cpp"
	"Measurement.cpp"
	"IntrusiveSharedPointer/IntrusiveShpFreeList.cpp"
	"ScopeParams.cpp"
	"ChannelObject.cpp"
	"Dispatcher.cpp"
	"DelayVisualizer.cpp"
	"FlashProgrammer.cpp"
	"ClockGen.cpp"
	"CalDacCtrl.cpp"
	"VersaClkDbg.cpp"
)

set(LIBS ${QWT} ${QT_LIBS} fwLib fwcomm ${FFTW3})
if (HDF5_FOUND)
	list(APPEND LIBS ${HDF5_LIBRARIES})
	add_compile_definitions( CONFIG_WITH_HDF5=1 )
endif()
if (JANSSON_FOUND)
	list(APPEND LIBS ${JANSSON_LIBRARIES})
	add_compile_definitions( CONFIG_WITH_JANSSON=1 )
endif()

include_directories(./ IntrusiveSharedPointer fwcommCPP usbadc-support/sw ${QWT_INC_PATH})

add_executable(scope ${SRCS})
set_target_properties(scope PROPERTIES AUTOMOC ON)

target_link_libraries(scope PRIVATE ${LIBS})

add_executable(flashTool flashTool.cpp)
target_link_libraries(flashTool PRIVATE fwLib fwcomm)
