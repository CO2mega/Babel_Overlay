find_package(wil CONFIG QUIET)
if(NOT wil_FOUND)
    message(WARNING "WIL package not found. Audio capture interface will be disabled.")
    return()
endif()

add_subdirectory(iaudiocapture)
