# main/ 子模块 — 由根 CMakeLists.txt 通过 add_subdirectory(main) 加载
#
# 内容：
#   - 业务入口 (app_main.c)
#   - 日志走独立 stm_log 库（Lib/stm_log），由 target_link_libraries 链入

if (TARGET ${CMAKE_PROJECT_NAME})
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/app_main.c
    )
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE stm_log)
else()
    message(STATUS "main/ not loaded by top-level CMake. Skipping app_main build.")
endif()