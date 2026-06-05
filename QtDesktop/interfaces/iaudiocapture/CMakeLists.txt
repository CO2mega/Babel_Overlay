if(NOT WIN32)
    message(WARNING "Audio capture is only supported on Windows. Skipping.")
    return()
endif()

# ===== Audio Capture Shared Library (DLL) =====
add_library(audio_capture SHARED
    audio_capture_dll.cpp
    system_audio_capture.cpp
)

set_target_properties(audio_capture PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_compile_definitions(audio_capture PRIVATE
    NOMINMAX
    AUDIO_CAPTURE_EXPORTS
)

target_include_directories(audio_capture PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(audio_capture PUBLIC
    WIL::WIL
    mmdevapi
    mfplat
    ksuser
    ole32
    oleaut32
)

# ===== Example Console Application =====
# add_executable(audio_capture_example
#     main.cpp
# )

# target_compile_definitions(audio_capture_example PRIVATE NOMINMAX)

# target_include_directories(audio_capture_example PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}
# )

# target_link_libraries(audio_capture_example PRIVATE
#     audio_capture
# )
