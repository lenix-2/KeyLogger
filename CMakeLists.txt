cmake_minimum_required(VERSION 3.15)
project(SystemService)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Опции компиляции для уменьшения детектирования
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -s -fno-rtti -fno-exceptions")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GL /Gy /Os")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG")
endif()

add_executable(system_service system_service.cpp)
target_link_libraries(system_service wininet ws2_32)
