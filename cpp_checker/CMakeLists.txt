cmake_minimum_required(VERSION 3.10)
project(keyboard_checker)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source files
add_executable(keyboard_checker
    main.cpp
    keyboard_checker.cpp
)

# Add Windows libraries
target_link_libraries(keyboard_checker
    comctl32
    user32
)
