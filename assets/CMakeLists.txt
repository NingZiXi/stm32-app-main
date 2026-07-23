if(TARGET ${CMAKE_PROJECT_NAME})
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/app_main.c
    )
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
    target_link_libraries(${CMAKE_PROJECT_NAME} stm_log)
    if(TARGET segger_rtt)
        target_link_libraries(${CMAKE_PROJECT_NAME} segger_rtt)
    endif()
else()
    message(STATUS "main/ not loaded by top-level CMake. Skipping app_main build.")
endif()