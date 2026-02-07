cmake_minimum_required(VERSION 4.1)
project(TrafficControlSystem)

set(CMAKE_CXX_STANDARD 20)

add_executable(
        TrafficControlSystem
        main.cpp
        #Test/DataValidation/DataValidation.cpp
        TrafficControlSystem.hpp
        TrafficControlSystem.cpp
        Mediator.hpp
        Semaphore/Semaphore.hpp
        Semaphore/Semaphore.cpp
        PedestrianSemaphore/Buzzer/Buzzer.cpp
        PedestrianSemaphore/PedestrianSemaphore.cpp
        PedestrianSemaphore/Buzzer/PWM_DeviceDriver.cpp
        GPIOHandling/rasp_gpio.cpp
        TrafficSemaphore/TrafficSemaphore.cpp
        Messages/Components/PedestrianEvent.hpp
        Messages/Components/Cloud/QueueSendCloudTypes.hpp
        Mediator.cpp
        TrafficStrategy/TrafficStrategy.hpp
        TrafficStrategy/Normal_TrafficStrategy.cpp
        PedestrianSemaphore/RFID/MFRC522.cpp
        PedestrianSemaphore/RFID/MFRC522.hpp
        PedestrianSemaphore/RFID/SPI_DeviceDriver.cpp
        PedestrianSemaphore/RFID/SPI_DeviceDriver.hpp
        PedestrianSemaphore/Button/Button.cpp
        PedestrianSemaphore/Button/Button.hpp
        Messages/Components/DDSEvent.hpp
        Messages/Components/Cloud/QueueReceiveCloudTypes.hpp
        Messages/EventsType.hpp
        CppWrapper/CppWrapper.hpp
        CppWrapper/CondVar_CppWrapper.cpp
        CppWrapper/MQueue_CppWrapper.cpp
        CppWrapper/Thread_CppWrapper.cpp
        CppWrapper/Mutex_CppWrapper.cpp
        CppWrapper/Timer_CppWrapper.cpp
        CloudInterface/CloudInterface.cpp
        CloudInterface/CloudInterface.hpp
        TrafficStrategy/SetUp_TrafficStrategy.cpp
        TrafficStrategy/Emergency_TrafficStrategy.cpp
        TrafficStrategy/Failure_StrategyEmergency.cpp
      #  PedestrianSemaphore/LinuxDeviceDriver/pwm_dd.h
      #  PedestrianSemaphore/LinuxDeviceDriver/pwm_dd.c
      #  PedestrianSemaphore/LinuxDeviceDriver/utility.c
      #  PedestrianSemaphore/LinuxDeviceDriver/utility.h
        Subscriber/DDSSubscriber.cpp
        Subscriber/EmergencyMSGPubSubTypes.cxx
        Subscriber/EmergencyMSGTypeObjectSupport.cxx
        Messages/InternalEvent.hpp
)

target_link_libraries(TrafficControlSystem gpiod
        /home/andre/buildroot3/buildroot-2025.02.4/output/host/aarch64-buildroot-linux-gnu/sysroot/usr/lib/libcurl.so
        /home/andre/buildroot3/buildroot-2025.02.4/output/host/aarch64-buildroot-linux-gnu/sysroot/usr/lib/libfastdds.so
        /home/andre/buildroot3/buildroot-2025.02.4/output/host/aarch64-buildroot-linux-gnu/sysroot/usr/lib/libfastcdr.so
)


