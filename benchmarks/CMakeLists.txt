cmake_minimum_required(VERSION 2.8)

add_executable(timer_event_loop EXCLUDE_FROM_ALL timer_event_loop.cc)
target_link_libraries(timer_event_loop ten)

add_executable(server_client EXCLUDE_FROM_ALL server_client.cc)
target_link_libraries(server_client ten)
