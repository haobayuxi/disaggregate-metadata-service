# Author: Ming Zhang
# Copyright (c) 2022

set(DB_SOURCES micro_db.cc)
set(TXN_SOURCES micro_meta.cc)

add_library(micro_db STATIC ${DB_SOURCES})
add_library(micro_meta STATIC ${TXN_SOURCES})

set_target_properties(micro_db PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(micro_meta PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(micro_meta ford)