cmake_minimum_required(VERSION 3.10)

project(gimp-max-plugin)
set(PLUGIN_BINARY "file-max")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Release)

enable_language(C)

set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})

set(BIN_DIR ${PROJECT_SOURCE_DIR}/bin)
set(LIB_DIR ${PROJECT_SOURCE_DIR}/lib)

add_subdirectory(src)

INCLUDE(FindPkgConfig)
PKG_SEARCH_MODULE(GIMP REQUIRED gimp-2.0)
PKG_SEARCH_MODULE(GIMPUI REQUIRED gimpui-2.0)
PKG_SEARCH_MODULE(GTK+ REQUIRED gtk+-2.0)

add_executable(${PLUGIN_BINARY} ${APP_SOURCE_FILES})
target_compile_definitions(${PLUGIN_BINARY} PUBLIC GIMP_DISABLE_DEPRECATED GTK_DISABLE_DEPRECATED)
target_include_directories(${PLUGIN_BINARY} PUBLIC ${GIMP_INCLUDE_DIRS} ${GIMPUI_INCLUDE_DIRS} ${GTK+_INCLUDE_DIRS})
target_link_directories(${PLUGIN_BINARY} PUBLIC ${LIB_DIR})
target_link_libraries(${PLUGIN_BINARY} ${GIMP_LIBRARIES} ${GIMPUI_LIBRARIES} ${GTK+_LIBRARIES} )
