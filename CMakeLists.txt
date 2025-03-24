cmake_minimum_required (VERSION 3.4)
set(CMAKE_BUILD_TYPE "Debug")
project(output LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_PREFIX_PATH "~/Qt/6.9.0/gcc_64")


find_package(PkgConfig REQUIRED)
find_package(SDL2 REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libavformat libavcodec libavutil libswscale libswresample)
find_package(Qt6 COMPONENTS Core Widgets REQUIRED)
# pkg_check_modules(SDL2 REQUIRED sdl2)
include_directories(${FFMPEG_INCLUDE_DIRS})
include_directories(${SDL2_INCLUDE_DIRS})
include_directories(${QT_INCLUDE_DIRS})

link_directories("/usr/local/lib")
# link_directories(${SDL2_LIBRARY_DIRS})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_library(GTEST_LIB gtest PATHS /usr/local/lib)
find_library(GTEST_MAIN_LIB gtest_main PATHS /usr/local/lib)
find_library(GMOCK_LIB gmock PATHS /usr/local/lib)
find_library(GMOCK_MAIN_LIB gmock_main PATHS /usr/local/lib)
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
file(GLOB_RECURSE SRCS "${SRC_DIR}/*.cpp")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/usr/include/c++/11")
add_executable(${PROJECT_NAME} main.cpp ${SRCS})
target_link_libraries(${PROJECT_NAME} 
                        ${GTEST_LIB} 
                        ${GTEST_MAIN_LIB} 
                        ${GMOCK_LIB} 
                        ${GMOCK_MAIN_LIB}
                        ${FFMPEG_LIBRARIES}
                        ${SDL2_LIBRARIES}
                        Qt6::Widgets)                  
target_include_directories(${PROJECT_NAME} PRIVATE /usr/local/include)
link_directories(${FFMPEG_LIBRARY_DIRS})
# add_executable(${PROJECT_NAME} main.cpp)