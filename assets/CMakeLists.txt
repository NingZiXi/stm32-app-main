if(TARGET ${CMAKE_PROJECT_NAME})
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/app_main.c
    )
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
    # stm_log v3 在开启 STM_LOG_WITH_RTT 时通过 PUBLIC 依赖传递 RTT。
    target_link_libraries(${CMAKE_PROJECT_NAME} stm_log)
else()
    message(STATUS "main/ not loaded by top-level CMake. Skipping app_main build.")
endif()
