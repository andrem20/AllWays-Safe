cmake_minimum_required(VERSION 3.20)

project(Subscriber VERSION 1 LANGUAGES CXX)

# Find requirements
if(NOT fastcdr_FOUND)
    find_package(fastcdr 2 REQUIRED)
endif()

if(NOT fastdds_FOUND)
    find_package(fastdds 3 REQUIRED)
endif()

# Check C++11
include(CheckCXXCompilerFlag)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    check_cxx_compiler_flag(-std=c++11 SUPPORTS_CXX11)
    if(NOT SUPPORTS_CXX11)
        message(FATAL_ERROR "Compiler doesn't support C++11")
    endif()
endif()

# Set CMAKE_BUILD_TYPE to Release by default
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE Release CACHE STRING
            "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
            FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

message(STATUS "Configuring publisher...")

# Source files
set(PUBLISHER_SOURCES
        main.cpp
        DDSSubscriber.cpp
        EmergencyMSGPubSubTypes.cxx
        EmergencyMSGTypeObjectSupport.cxx
)

add_executable(Subscriber ${PUBLISHER_SOURCES})

target_compile_definitions(Subscriber PRIVATE
        $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
        $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG>
        $<$<BOOL:${SHM_TRANSPORT_DEFAULT}>:SHM_TRANSPORT_BUILTIN>
)

target_link_libraries(Subscriber
        /home/mariana/buildroot/buildroot-2025.02.4/output/host/aarch64-buildroot-linux-gnu/sysroot/usr/lib/libfastdds.so
        /home/mariana/buildroot/buildroot-2025.02.4/output/host/aarch64-buildroot-linux-gnu/sysroot/usr/lib/libfastcdr.so
        /home/mariana/buildroot/buildroot-2025.02.4/output/host/aarch64-buildroot-linux-gnu/sysroot/usr/lib/libtinyxml2.so.11
        /home/mariana/buildroot/buildroot-2025.02.4/output/host/aarch64-buildroot-linux-gnu/sysroot/usr/lib/libfoonathan_memory-0.7.4.so
)


# Copy XML files
file(GLOB_RECURSE XML_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.xml)
foreach(XML_FILE_COMPLETE_PATH ${XML_FILES})
    get_filename_component(XML_FILE ${XML_FILE_COMPLETE_PATH} NAME_WE)
    configure_file(
            ${XML_FILE_COMPLETE_PATH}
            ${CMAKE_CURRENT_BINARY_DIR}/${XML_FILE}.xml
            COPYONLY)
endforeach()