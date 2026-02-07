cmake_minimum_required(VERSION 3.30)
project(evcontrolsystem)

set(CMAKE_CXX_STANDARD 20)

include_directories(
        ${CMAKE_SOURCE_DIR}
        /home/mariana/buildroot/buildroot-2025.02.8/output/host/aarch64-buildroot-linux-gnu/sysroot/usr/include
)

add_executable(evcontrolsystem main.cpp
                Battery/ADS1115.cpp
                PWM_DeviceDriver/PWM_DeviceDriver.cpp
                Battery/Battery.cpp
                Publisher/DDSPublisher.cpp
                Publisher/EmergencyMSGPubSubTypes.cxx
                Publisher/EmergencyMSGTypeObjectSupport.cxx
                CloudInterface/CloudInterface.cpp
                CppWrapper_pthreads/CondVar_CppWrapper.cpp
                CppWrapper_pthreads/Mutex_CppWrapper.cpp
                CppWrapper_pthreads/MQueue_CppWrapper.cpp
                CppWrapper_pthreads/Timer_CppWrapper.cpp
                CppWrapper_pthreads/Thread_CppWrapper.cpp
                evcontrolsystem.cpp
                evcontrolsystem.hpp)

target_link_libraries(evcontrolsystem
        /home/mariana/buildroot/buildroot-2025.02.8/output/host/aarch64-buildroot-linux-gnu/sysroot/usr/lib/libfastdds.so
        /home/mariana/buildroot/buildroot-2025.02.8/output/host/aarch64-buildroot-linux-gnu/sysroot/usr/lib/libfastcdr.so
        /home/mariana/buildroot/buildroot-2025.02.8/output/host/aarch64-buildroot-linux-gnu/sysroot/usr/lib/libcurl.so
)