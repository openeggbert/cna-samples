# cmake/SampleHelpers.cmake
# Helpers for registering CNA sample executables.

# cna_add_sample(target_name
#     SOURCES src/Foo.cpp src/Bar.cpp
#     [CONTENT_DIR path/to/Content]   # optional: copies assets next to the exe
# )
#
# Creates an executable named `<target_name>_cna_samples` (use exact directory
# name, e.g. SafeArea, Platformer), links it against the CNA framework and
# (on GNU/Clang Linux) wraps the link in a linker group to resolve circular
# references between CNA and the graphics backend.
function(cna_add_sample target_name)
    cmake_parse_arguments(ARG "" "CONTENT_DIR" "SOURCES" ${ARGN})

    set(full_target "${target_name}_cna_samples")

    add_executable(${full_target} ${ARG_SOURCES})

    target_include_directories(${full_target} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    # Link against CNA; use a linker group on GCC/Clang Linux to handle
    # circular references between CNA and the EasyGL/Vulkan backend.
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND NOT WIN32 AND NOT EMSCRIPTEN)
        if(DEFINED BACKEND_TARGET AND TARGET ${BACKEND_TARGET})
            target_link_libraries(${full_target} PRIVATE
                -Wl,--start-group CNA ${BACKEND_TARGET} -Wl,--end-group
                SHARP_RUNTIME)
        else()
            target_link_libraries(${full_target} PRIVATE CNA SHARP_RUNTIME)
        endif()
    elseif(EMSCRIPTEN)
        target_link_libraries(${full_target} PRIVATE
            CNA SDL3::SDL3-static SHARP_RUNTIME)
        set_target_properties(${full_target} PROPERTIES SUFFIX ".html")
        target_link_options(${full_target} PRIVATE
            -sALLOW_MEMORY_GROWTH=1
            -sFORCE_FILESYSTEM=1
            "-sMIN_WEBGL_VERSION=2"
            "-sMAX_WEBGL_VERSION=2"
        )
    else()
        target_link_libraries(${full_target} PRIVATE CNA SHARP_RUNTIME)
    endif()

    if(TARGET SDL3::SDL3main)
        target_link_libraries(${full_target} PRIVATE SDL3::SDL3main)
    endif()

    # CNA_Net (NetworkSession, PacketWriter/Reader, ...) pulls in CNA_GamerServices
    # (GamerServicesComponent, Guide, SignedInGamer, ...) transitively via its own
    # PUBLIC link — only samples that actually call into Microsoft::Xna::Framework::Net
    # need this, but linking it unconditionally when the target exists is harmless.
    if(TARGET CNA_Net)
        target_link_libraries(${full_target} PRIVATE CNA_Net)
    endif()

    if(WIN32)
        set_target_properties(${full_target} PROPERTIES WIN32_EXECUTABLE TRUE)
        if(COMMAND cna_copy_sdl_runtime)
            cna_copy_sdl_runtime(${full_target})
        endif()
    endif()

    # Copy Content/ assets next to the built executable.
    if(ARG_CONTENT_DIR)
        add_custom_command(TARGET ${full_target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${ARG_CONTENT_DIR}"
                "$<TARGET_FILE_DIR:${full_target}>/Content"
            VERBATIM
        )
    endif()
endfunction()
