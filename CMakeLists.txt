cmake_minimum_required(VERSION 3.16)
project(binred)

if(WIN32)
link_directories("D:/OpenSSL/build-x64-debug/lib")
else()
# find_package(OPENSSL REQUIRED)
link_directories("/lib/x86_64-linux-gnu")
endif()

include_directories("src/commonlib" "D:/Minitools/Socketlib/src/v1" "D:/OpenSSL/build-x64/include")

set(CMAKE_CXX_STANDARD 20)

find_package(Threads REQUIRED)

add_executable(binred "src/main.cpp")

#add_executable(wsserver "src/webserver.cpp")

#if(WIN32)
#target_link_libraries(wsserver libssl libcrypto ws2_32)
#else()
#target_link_libraries(wsserver libssl.so libcrypto.so Threads::Threads)
#endif()




