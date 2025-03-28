set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
cmake_minimum_required(VERSION 3.7.0)
project(screenrec)
add_compile_definitions(_GNU_SOURCE)
add_compile_options(-lpthread -O3 -ftree-vectorizer-verbose=5)


file(GLOB_RECURSE SOURCE_FILES
${CMAKE_SOURCE_DIR}/src/*.c
${CMAKE_SOURCE_DIR}/src/*.cpp)

file(GLOB_RECURSE HEADER_FILES
${CMAKE_SOURCE_DIR}/include/*.h
${CMAKE_SOURCE_DIR}/include/*.hpp)

find_library(MATH_LIBRARY m)
find_library(DL_LIBRARY, dl)

set(LIBS rt xcb xcb-util xcb-image avcodec avutil ${MATH_LIBRARY} ${DL_LIBRARY})

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(${PROJECT_NAME} ${LIBS})