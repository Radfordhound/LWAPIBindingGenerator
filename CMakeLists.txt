# Setup CMake
cmake_minimum_required(VERSION 3.13...3.22)

# Setup project
project(LWAPIBindingGenerator)

# Import LibclangStatic
list(APPEND CMAKE_PREFIX_PATH
    "${PROJECT_SOURCE_DIR}/depends/libclang-static-build"
)

find_library(LIBCLANG_STATIC_LIBRARY clang_static_bundled REQUIRED)
find_path(LIBCLANG_STATIC_INCLUDE_DIR "clang-c/Index.h" REQUIRED)

# Setup executable
set(SOURCE_DIR "src")
add_executable(LWAPIBindingGenerator
    "${SOURCE_DIR}/LWAPIBindingGenerator.c"
    "${SOURCE_DIR}/pch.h"
)

target_precompile_headers(LWAPIBindingGenerator
    PRIVATE "${SOURCE_DIR}/pch.h"
)

target_include_directories(LWAPIBindingGenerator
    PRIVATE ${LIBCLANG_STATIC_INCLUDE_DIR}
)

target_compile_definitions(LWAPIBindingGenerator PUBLIC -DCINDEX_NO_EXPORTS)
target_link_libraries(LWAPIBindingGenerator ${LIBCLANG_STATIC_LIBRARY} Version)
target_link_options(LWAPIBindingGenerator PUBLIC /NODEFAULTLIB:libcmt.lib)

install(TARGETS LWAPIBindingGenerator
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES
    ${PROJECT_SOURCE_DIR}/LICENSE.txt
    ${PROJECT_SOURCE_DIR}/THIRD-PARTY-LICENSES.txt
    DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/lwapibindinggenerator
)
