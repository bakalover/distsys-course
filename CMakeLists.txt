cmake_minimum_required(VERSION 3.14)

project(distsys_course)

include(cmake/Ccache.cmake)
include(cmake/Helpers.cmake)
include(cmake/CheckCompiler.cmake)
include(cmake/CompileOptions.cmake)
include(cmake/Sanitize.cmake)

add_subdirectory(library)

include(cmake/Task.cmake)
add_subdirectory(tasks)
