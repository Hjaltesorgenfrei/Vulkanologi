cmake_minimum_required(VERSION 3.15)
project(yojimbo)

set(CMAKE_CXX_STANDARD 14)

CPMAddPackage("gh:robinlinden/libsodium-cmake#f568ff02f1bed155ea598c0e803ef3c9db2703d2")
CPMAddPackage("gh:Mbed-TLS/mbedtls@3.2.1")

add_library(yojimbo
        yojimbo/yojimbo.cpp yojimbo/yojimbo.h
        yojimbo/netcode.io/netcode.c yojimbo/netcode.io/netcode.h
        yojimbo/reliable.io/reliable.c yojimbo/reliable.io/reliable.h
        yojimbo/certs.c yojimbo/certs.h
        yojimbo/tlsf/tlsf.c yojimbo/tlsf/tlsf.c)
target_link_libraries(yojimbo
    PRIVATE MbedTLS::mbedtls
           MbedTLS::mbedcrypto
           MbedTLS::mbedx509 
           sodium)
target_compile_definitions(yojimbo PUBLIC 
        NETCODE_ENABLE_TESTS=0 
        RELIABLE_ENABLE_TESTS=0)    
target_include_directories(yojimbo PUBLIC yojimbo/netcode.io yojimbo/reliable.io yojimbo)

set(YOJIMBO_LIBRARY yojimbo CACHE STRING "" FORCE)
mark_as_advanced(YOJIMBO_LIBRARY)