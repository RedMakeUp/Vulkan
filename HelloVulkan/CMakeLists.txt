cmake_minimum_required(VERSION 3.16)

project(HelloVulkan VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(SOURCES 
    Application.h 
    Application.cpp
    main.cpp 
    )

add_executable(HelloVulkan)
target_sources(HelloVulkan PRIVATE ${SOURCES})

find_package(glfw3 3.2 REQUIRED)
target_link_libraries(HelloVulkan glfw)

find_package(Vulkan REQUIRED)
target_include_directories(HelloVulkan PRIVATE Vulkan::Vulkan)
target_link_libraries(HelloVulkan Vulkan::Vulkan)

