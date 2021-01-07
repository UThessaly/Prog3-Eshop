cmake_minimum_required(VERSION 3.13.4)
project(better_eshop VERSION 0.1.0)

include(CTest)
enable_testing()

set(CMAKE_CXX_STANDARD 20)

if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
   message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
   file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/master/conan.cmake"
                  "${CMAKE_BINARY_DIR}/conan.cmake")
endif()

include(${CMAKE_BINARY_DIR}/conan.cmake)

conan_cmake_run(REQUIRES 
                    spdlog/1.8.2                        
                    docopt.cpp/0.6.3 
                    yaml-cpp/0.6.3
                    sol2/3.2.3
                    nlohmann_json/3.9.1
                BASIC_SETUP CMAKE_TARGETS)


add_executable(better_eshop src/main.cpp)
target_link_libraries(better_eshop PUBLIC CONAN_PKG::nlohmann_json CONAN_PKG::spdlog CONAN_PKG::docopt.cpp CONAN_PKG::yaml-cpp CONAN_PKG::sol2)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
