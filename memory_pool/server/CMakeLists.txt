# Author: Ming Zhang
# Copyright (c) 2022

set(SERVER_SOURCE server.cc)
add_executable(zm_mem_pool ${SERVER_SOURCE})
target_link_libraries(zm_mem_pool micro_db rlib)


# add_executable(infinifs_server infinifs_server.cc)
# target_link_libraries(infinifs_server micro_db rlib)