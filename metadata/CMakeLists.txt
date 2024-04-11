


add_library(dtx_tpcc dtx.cpp drtm.cpp dslr.cpp)
target_compile_definitions(dtx_tpcc PUBLIC CONFIG_ITEM_SIZE=664)
target_link_libraries(dtx_tpcc PUBLIC smart)


add_subdirectory(taobench)

