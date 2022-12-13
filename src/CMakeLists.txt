cmake_minimum_required(VERSION 3.23)
project(Computer_Network_Project C)

set(CMAKE_C_STANDARD 99)

add_executable(Receiver receiver.c struct_define.h print_utils.h checksum_utils.h)
add_executable(Sender sender.c struct_define.h checksum_utils.h print_utils.h)