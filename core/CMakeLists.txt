# Author: Ming Zhang
# Copyright (c) 2022

set(CONNECTION_SRC
        connection/meta_manager.cc
        connection/qp_manager.cc
        )


set(DMC_SRC
        metadata/dmc_check_ro.cc
        metadata/dmc_check.cc
        metadata/dmc_issue.cc
        metadata/dmc.cc
        metadata/doorbell.cc
        )



set(SCHEDULER_SRC
        scheduler/corotine_scheduler.cc
        )

add_library(ford STATIC
        ${CONNECTION_SRC}
        ${SCHEDULER_SRC}
        ${DMC_SRC}
        )

set_target_properties(ford PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(ford rlib pthread boost_coroutine boost_context boost_system)

