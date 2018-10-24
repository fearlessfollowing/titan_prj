cmake_minimum_required(VERSION 3.6)
project(mongoose)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -fexceptions -pie -fPIE -Wall -Wunused-variable -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC -frtti -fexceptions -pie -fPIE -Wall -Wunused-variable -Werror")
set(SRC_FILES mongoose.cpp simplest_web_server.cpp)
add_executable(${PROJECT_NAME} ${SRC_FILES})

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})

