cmake_minimum_required(VERSION 3.23)
project(HW4 C)

set(CMAKE_C_STANDARD 99)

set(CMAKE_LIBRARY_PATH /usr/include/openssl)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0")

set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)

add_executable(HW4 task.c ${SOURCE_FILES})
target_link_libraries(HW4 OpenSSL::Crypto)