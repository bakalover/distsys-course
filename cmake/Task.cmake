set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin")

# --------------------------------------------------------------------

set(LIBS_LIST "wheels")

# --------------------------------------------------------------------

# Helpers

macro(get_task_target VAR NAME)
    set(${VAR} task_${HW_NAME}_${TASK_NAME}_${NAME})
endmacro()

function(add_task_executable BINARY_NAME)
    set(BINARY_SOURCES ${ARGN})

    add_executable(${BINARY_NAME} ${BINARY_SOURCES} ${TASK_SOURCES})
    target_link_libraries(${BINARY_NAME} pthread ${LIBS_LIST})
    add_dependencies(${BINARY_NAME} ${LIBS_LIST})
endfunction()

# --------------------------------------------------------------------

# Prologue

macro(begin_task)
    set(TASK_DIR ${CMAKE_CURRENT_SOURCE_DIR})

    get_filename_component(TASK_NAME ${TASK_DIR} NAME)

    get_filename_component(HW_DIR ${TASK_DIR} DIRECTORY)
    get_filename_component(HW_NAME ${HW_DIR} NAME)

    message(STATUS "Homework = '${HW_NAME}', task = '${TASK_NAME}'")

    include_directories(${TASK_DIR})

    set(TEST_LIST "")
endmacro()

# --------------------------------------------------------------------

# Dependencies

macro(task_link_libraries)
    list(APPEND LIBS_LIST ${ARGV})
endmacro()

# --------------------------------------------------------------------

# Sources

macro(set_task_sources)
    prepend(TASK_SOURCES "${TASK_DIR}/" ${ARGV})
endmacro()

# --------------------------------------------------------------------

# Libraries

function(add_task_library DIR_NAME)
    # Optional lib target name (dir name by default)
    if (${ARGC} GREATER 1)
        set(LIB_NAME ${ARGV1})
    else()
        set(LIB_NAME ${DIR_NAME})
    endif()

    set(LIB_DIR ${TASK_DIR}/${DIR_NAME})

    get_task_target(LIB_TARGET ${LIB_NAME})
    message(STATUS "Add task library target = ${LIB_TARGET}")

    # Library
    file(GLOB_RECURSE LIB_CXX_SOURCES ${LIB_DIR}/*.cpp)
    file(GLOB_RECURSE LIB_HEADERS ${LIB_DIR}/*.hpp ${LIB_DIR}/*.ipp)
    add_library(${LIB_TARGET} STATIC ${LIB_CXX_SOURCES} ${LIB_HEADERS})

    # Include dir
    get_filename_component(LIB_INCLUDE_DIR "${LIB_DIR}/.." ABSOLUTE)
    target_include_directories(${LIB_TARGET} PUBLIC ${LIB_INCLUDE_DIR})

    # Dependencies
    target_link_libraries(${LIB_TARGET} ${LIBS_LIST})

    # Append ${LIB_TARGET to LIBS_LIST
    list(APPEND LIBS_LIST ${LIB_TARGET})
    set(LIBS_LIST ${LIBS_LIST} PARENT_SCOPE)
endfunction()

# --------------------------------------------------------------------

# Playground

function(add_playground DIR_NAME)
    get_task_target(PLAY_NAME "playground")

    set(PLAY_DIR "${TASK_DIR}/${DIR_NAME}")
    file(GLOB_RECURSE PLAY_CXX_SOURCES ${PLAY_DIR}/*.cpp)

    add_task_executable(${PLAY_NAME} ${PLAY_CXX_SOURCES})
endfunction()

# --------------------------------------------------------------------

# Tests

function(add_task_test BINARY_NAME)
    get_task_target(TEST_NAME ${BINARY_NAME})

    prepend(TEST_SOURCES "${TASK_DIR}/" ${ARGN})
    add_task_executable(${TEST_NAME} ${TEST_SOURCES})

    # Append test to TEST_LIST
    list(APPEND TEST_LIST ${TEST_NAME})
    set(TEST_LIST "${TEST_LIST}" PARENT_SCOPE)
endfunction()

function(add_task_test_dir DIR_NAME)
    # Optional test target name (dir name by default)
    if (${ARGC} GREATER 1)
        set(BINARY_NAME ${ARGV1})
    else()
        set(BINARY_NAME ${DIR_NAME})
    endif()

    get_task_target(TEST_NAME ${BINARY_NAME})

    set(TEST_DIR "${TASK_DIR}/${DIR_NAME}")
    file(GLOB_RECURSE TEST_CXX_SOURCES ${TEST_DIR}/*.cpp)

    add_task_executable(${TEST_NAME} ${TEST_CXX_SOURCES})

    # Append test to TEST_LIST
    list(APPEND TEST_LIST ${TEST_NAME})
    set(TEST_LIST "${TEST_LIST}" PARENT_SCOPE)
endfunction()

function(add_task_all_tests_target)
    get_task_target(ALL_TESTS_TARGET "run_all_tests")
    run_chain(${ALL_TESTS_TARGET} ${TEST_LIST})
endfunction()

# --------------------------------------------------------------------

# Benchmark

function(add_task_benchmark BINARY_NAME)
    get_task_target(BENCH_NAME ${BINARY_NAME})

    prepend(BENCH_SOURCES "${TASK_DIR}/" ${ARGN})
    add_task_executable(${BENCH_NAME} ${BENCH_SOURCES})
    target_link_libraries(${BENCH_NAME} benchmark)

    if(${TOOL_BUILD})
        get_task_target(RUN_BENCH_TARGET "run_benchmark")
        add_custom_target(${RUN_BENCH_TARGET} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${BENCH_NAME})
        add_dependencies(${RUN_BENCH_TARGET} ${BENCH_NAME})
    endif()
endfunction()

# --------------------------------------------------------------------

# Epilogue

function(end_task)
    if(${TOOL_BUILD})
        add_task_all_tests_target()
    endif()
endfunction()
