
if(NOT TARGET retdec::anchor_emitter)
    find_package(retdec @PROJECT_VERSION@
        REQUIRED
        COMPONENTS
            config
            common
            rapidjson
    )

    include(${CMAKE_CURRENT_LIST_DIR}/retdec-anchor-emitter-targets.cmake)
endif()
