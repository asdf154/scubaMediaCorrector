cmake_minimum_required(VERSION 3.20)
project(scuba_media_corrector VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(scuba_corrector
    src/ColorCorrection.cpp
)
target_include_directories(scuba_corrector PUBLIC include)

add_executable(scuba_corrector_cli
    src/cli.cpp
)
target_link_libraries(scuba_corrector_cli PRIVATE scuba_corrector)

include(CTest)
if(BUILD_TESTING)
    add_executable(scuba_corrector_tests
        tests/test_color_correction.cpp
    )
    target_link_libraries(scuba_corrector_tests PRIVATE scuba_corrector)
    add_test(NAME scuba_corrector_tests COMMAND scuba_corrector_tests)
endif()
