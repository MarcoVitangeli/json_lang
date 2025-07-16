cmake_minimum_required(VERSION 3.31)
project(json_lang)

include(FetchContent)

FetchContent_Declare(
        simdjson
        GIT_REPOSITORY https://github.com/simdjson/simdjson.git
        GIT_SHALLOW TRUE
        GIT_TAG tags/v1.0.0
)

FetchContent_MakeAvailable(simdjson)

set(CMAKE_CXX_STANDARD 23)

add_executable(json_lang main.cpp)

target_link_libraries(json_lang simdjson)


