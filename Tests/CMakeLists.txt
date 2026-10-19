cmake_minimum_required(VERSION 3.22)

add_executable(tests_runner
    test_main.cpp
    test_passthrough.cpp
    test_yin.cpp
    test_database.cpp
)

target_link_libraries(tests_runner PRIVATE juce::juce_core)
target_include_directories(tests_runner PRIVATE ../Source)

# For CI to pick up with ctest
include(CTest)
add_test(NAME tests_runner COMMAND tests_runner)
