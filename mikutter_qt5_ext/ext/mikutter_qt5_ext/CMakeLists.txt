cmake_minimum_required(VERSION 3.10)
project(mikutter_qt5_ext)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Ruby REQUIRED)
find_package(Qt5 COMPONENTS Widgets REQUIRED)

execute_process(
  COMMAND ${Ruby_EXECUTABLE} -r rbconfig -e "print RbConfig::CONFIG['LIBRUBY']"
  OUTPUT_VARIABLE LIBRUBY_NAME
)

add_library(
  mikutter_qt5_ext
  MODULE
  mikutter.cpp
  mikutter.hpp
  init.cpp
  ui/MikutterWindow.cpp
  ui/MikutterWindow.hpp
)

set_target_properties(mikutter_qt5_ext PROPERTIES PREFIX "")
if(APPLE)
  set_target_properties(mikutter_qt5_ext PROPERTIES SUFFIX ".bundle")
endif()
target_include_directories(mikutter_qt5_ext PRIVATE ${Ruby_HDR_DIR} ${Ruby_ARCHHDR_DIR})
target_link_libraries(mikutter_qt5_ext ${Ruby_POSSIBLE_LIB_DIR}/${LIBRUBY_NAME} Qt5::Widgets)

install(TARGETS mikutter_qt5_ext DESTINATION ${CMAKE_INSTALL_PREFIX}/mikutter_qt5_ext)
