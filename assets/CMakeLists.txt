# main/ 子模块 — 由根 CMakeLists.txt 通过 add_subdirectory(main) 加载

if (TARGET ${CMAKE_PROJECT_NAME})
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/app_main.c
        ${CMAKE_CURRENT_SOURCE_DIR}/app_log.c
    )
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
else()
    message(STATUS "main/ not loaded by top-level CMake. Skipping app_main build.")
endif()