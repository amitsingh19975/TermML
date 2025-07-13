cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(termml CXX C)
include(cmake/StandardProjectSettings.cmake)

include_directories(include)

add_library(termml_project_options INTERFACE)
add_library(termml_project_warnings INTERFACE)
add_library(termml::project_options ALIAS termml_project_options)
add_library(termml::project_warnings ALIAS termml_project_warnings)

include(cmake/CompilerWarnings.cmake)
set_project_warnings(termml_project_warnings)

include(cmake/Sanitizers.cmake)
enable_sanitizers(termml_project_options)

include(cmake/StaticAnalyzers.cmake)
include(cmake/Emscripten.cmake)

option(ENABLE_PCH "Enable Precompiled Headers" OFF)
if(ENABLE_PCH)
    target_precompile_headers(ui_project_options INTERFACE <vector> <string> <map> <utility> <unordered_map> <unordered_set> <list>)
endif(ENABLE_PCH)

add_library(termml_core INTERFACE)
add_library(termml::core ALIAS termml_core)
target_include_directories(termml_core INTERFACE include)

option(ENABLE_TESTING "Enable Test Builds" ON)

if(ENABLE_TESTING)
    enable_testing()
    message("Building Tests. Be sure to check out test/ directory")
    add_subdirectory(test)
endif(ENABLE_TESTING)

add_subdirectory(examples)

