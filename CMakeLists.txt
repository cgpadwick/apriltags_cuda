include(ExternalProject)
include(ProcessorCount)

cmake_minimum_required(VERSION 3.16 FATAL_ERROR)  # Ensure user has a compatible version of CMake

project(ApriltagCuda LANGUAGES CUDA CXX)  # Set project name and specify the languages used

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Look for required packages.
find_package(CUDA REQUIRED)
find_package(glog REQUIRED)
find_package(GTest REQUIRED)
find_package(ZLIB REQUIRED)

#set(GLOG_INSTALL_DIR ${CMAKE_BINARY_DIR}/glog-install)
#set(GTEST_INSTALL_DIR ${CMAKE_BINARY_DIR}/gtest-install)
set(OPENCV_INSTALL_DIR ${CMAKE_BINARY_DIR}/OpenCV-install)
set(WPILIB_INSTALL_DIR ${CMAKE_BINARY_DIR}/wpilib-install)
set(CCCL_INSTALL_DIR ${CMAKE_BINARY_DIR}/cccl-install)
set(HTTPLIB_INSTALL_DIR ${CMAKE_BINARY_DIR}/httplib-install)
set(UWEBSOCKETS_INSTALL_DIR ${CMAKE_BINARY_DIR}/uwebsockets-install)
set(APRILTAG_INSTALL_DIR ${CMAKE_BINARY_DIR}/apriltag-install)

# Determine number of processors for the parallel builds.
if(NOT DEFINED NUM_PROCESSORS)
    ProcessorCount(NUM_PROCESSORS)
    if(NUM_PROCESSORS EQUAL 0)
        set(NUM_PROCESSORS 1)
    endif()
endif()

# TODO: get glog build working correctly.  Stuck on not including <glog/logging.h>
# correctly error.
# Add glog package
#ExternalProject_Add(
#    glog
#    PREFIX ${CMAKE_BINARY_DIR}/glog
#    GIT_REPOSITORY https://github.com/google/glog.git
#    GIT_TAG v0.7.1
#    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${GLOG_INSTALL_DIR}
#    BUILD_COMMAND make -j${NUM_PROCESSORS}
#    INSTALL_COMMAND make install
#)

# Add gtest package
#ExternalProject_Add(
#    gtest
#    PREFIX ${CMAKE_BINARY_DIR}/gtest
#    GIT_REPOSITORY https://github.com/google/googletest.git
#    GIT_TAG v1.14.0
#    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${GTEST_INSTALL_DIR}
#    BUILD_COMMAND make -j${NUM_PROCESSORS}
#    INSTALL_COMMAND make install
#)

# Add OPENCV package
ExternalProject_Add(
    OpenCV
    PREFIX ${CMAKE_BINARY_DIR}/OpenCV
    GIT_REPOSITORY https://github.com/opencv/opencv.git
    GIT_TAG 4.9.0
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${OPENCV_INSTALL_DIR}
    BUILD_COMMAND make -j${NUM_PROCESSORS}
    INSTALL_COMMAND make install
)

set(CMAKE_PREFIX_PATH ${OPENCV_INSTALL_DIR} ${CMAKE_PREFIX_PATH})
message(STATUS "OPENCV_INSTALL_DIR: ${OPENCV_INSTALL_DIR} CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

# Add wpilib package
ExternalProject_Add(
    wpilib
    PREFIX ${CMAKE_BINARY_DIR}/wpilib
    GIT_REPOSITORY https://github.com/wpilibsuite/allwpilib.git
    GIT_TAG v2024.3.2
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${WPILIB_INSTALL_DIR} -DWITH_JAVA=OFF -DWITH_GUI=OFF -DOpenCV_DIR=${OPENCV_INSTALL_DIR}/lib/cmake/opencv4
    BUILD_COMMAND make -j${NUM_PROCESSORS}
    INSTALL_COMMAND make install
)

# Add CCCL package
ExternalProject_Add(
    CCCL 
    PREFIX ${CMAKE_BINARY_DIR}/cccl
    GIT_REPOSITORY https://github.com/NVIDIA/cccl.git
    GIT_TAG v2.3.2
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)
ExternalProject_Get_Property(CCCL SOURCE_DIR)
set(CCCL_SOURCE_DIR ${SOURCE_DIR})

# Add cpp-httplib package
ExternalProject_Add(
    httplib
    PREFIX ${CMAKE_BINARY_DIR}/httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.16.0
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${HTTPLIB_INSTALL_DIR}
    BUILD_COMMAND make -j${NUM_PROCESSORS}
    INSTALL_COMMAND make install
)

# Add uWebSockets package
ExternalProject_Add(
    uwebsockets
    PREFIX ${CMAKE_BINARY_DIR}/uwebsockets
    GIT_REPOSITORY https://github.com/uNetworking/uWebSockets.git
    GIT_TAG v20.64.0
    CONFIGURE_COMMAND ""
    BUILD_IN_SOURCE 1
    BUILD_COMMAND make -f GNUmakefile
    INSTALL_COMMAND make -f GNUmakefile prefix=${UWEBSOCKETS_INSTALL_DIR} install
    	COMMAND ${CMAKE_COMMAND} -E make_directory ${UWEBSOCKETS_INSTALL_DIR}/lib
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/uwebsockets/src/uwebsockets/uSockets/uSockets.a ${UWEBSOCKETS_INSTALL_DIR}/lib
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/uwebsockets/src/uwebsockets/uSockets/src/libusockets.h ${UWEBSOCKETS_INSTALL_DIR}/include/uWebSockets
)
ExternalProject_Get_Property(uwebsockets SOURCE_DIR)
set(UWEBSOCKETS_SOURCE_DIR ${SOURCE_DIR})
message(STATUS "UWEBSOCKETS_SOURCE_DIR: ${UWEBSOCKETS_SOURCE_DIR}")
message(STATUS "UWEBSOCKETS_INSTALL_DIR: ${UWEBSOCKETS_INSTALL_DIR}")

add_library(USOCKETSLIB SHARED IMPORTED)
set_target_properties(USOCKETSLIB PROPERTIES IMPORTED_LOCATION ${UWEBSOCKETS_INSTALL_DIR}/lib/uSockets.a)

#set(USOCKETSLIB ${UWEBSOCKETS_SOURCE_DIR}/uSockets/src/uSockets.a)
#find_library(USOCKETSLIB uSockets.a 
#    PATHS ${UWEBSOCKETS_SOURCE_DIR}/uSockets/src/
#    REQUIRED)
#message(STATUS "USOCKETSLIB: ${USOCKETSLIB}")

# Add 971 apriltag package
ExternalProject_Add(
    apriltag
    PREFIX ${CMAKE_BINARY_DIR}/apriltag
    GIT_REPOSITORY https://github.com/cgpadwick/apriltag.git
    GIT_TAG 3.3.0 
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${APRILTAG_INSTALL_DIR} -DOpenCV_DIR=${OPENCV_INSTALL_DIR}
    BUILD_COMMAND make -j${NUM_PROCESSORS}
    INSTALL_COMMAND make install
)

add_dependencies(wpilib OpenCV)
add_dependencies(apriltag OpenCV wpilib CCCL httplib uwebsockets)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_CUDA_COMPILER)
    message(STATUS "CUDA Compiler: ${CMAKE_CUDA_COMPILER}")

    if(CMAKE_CUDA_COMPILER MATCHES "nvcc")
        message(STATUS "Using NVCC as CUDA compiler")
        if(CMAKE_BUILD_TYPE MATCHES "Debug")
            set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --std c++20 --expt-relaxed-constexpr -g")
        else()
            set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --std c++20 --expt-relaxed-constexpr -Xptxas -O3")
        endif()
    else()
        message(STATUS "Using a different CUDA compiler")
        if(CMAKE_BUILD_TYPE MATCHES "Debug")
            set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --std c++20 -g -gdwarf-4 -O0")
        else()
            set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --std c++20 -O3")
        endif()
    endif()
else()
    message(FATAL_ERROR "CUDA compiler not found. Please set CMAKE_CUDA_COMPILER.")
endif()

if(ENABLE_ASAN)
    message(STATUS "AddressSanitizer is enabled")
    set(ASAN_FLAGS "-fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS} ${ASAN_FLAGS}")
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} ${COMMON_FLAGS} ${ASAN_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${ASAN_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${ASAN_FLAGS}")
endif()

if(ENABLE_MSAN)
    message(STATUS "MemorySanitizer is enabled")
    set(MSAN_FLAGS "-fsanitize=memory -fsanitize-memory-track-origins=2 -fno-omit-frame-pointer -fsanitize-memory-use-after-dtor")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS} ${MSAN_FLAGS}")
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} ${COMMON_FLAGS} ${MSAN_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${MSAN_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${MSAN_FLAGS}")
endif()

# Gather all source files in the current directory
set(CUDA_LIB_SOURCES 
    apriltag_detect.cu 
    apriltag_gpu.cu
    cuda_frc971.cu
    labeling_allegretti_2019_BKE.cu
    line_fit_filter.cu
    points.cu
    threshold.cu
    apriltag_utils.cu
    video_processor.cu)

# Add a library with the above source files
add_library(apriltag_cuda ${CUDA_LIB_SOURCES})

add_dependencies(apriltag_cuda apriltag)

# Include directories for the compiler
include_directories( 
    ${APRILTAG_INSTALL_DIR}/include/apriltag
    ${APRILTAG_INSTALL_DIR}/include/apriltag/common/
    ${CCCL_SOURCE_DIR}/cub
    ${CCCL_SOURCE_DIR}/libcudacxx/include
    ${CCCL_SOURCE_DIR}/thrust
    ${OPENCV_INSTALL_DIR}/include/opencv4
    ${WPILIB_INSTALL_DIR}/include/ntcore
    ${WPILIB_INSTALL_DIR}/include/wpiutil
    ${HTTPLIB_INSTALL_DIR}/include
    ${UWEBSOCKETS_INSTALL_DIR}/include/uWebSockets)

# Add executable for OpenCV CUDA demo
add_executable(opencv_cuda_demo opencv_cuda_demo.cu)
target_link_libraries(opencv_cuda_demo 
    apriltag_cuda 
    ${APRILTAG_INSTALL_DIR}/lib/libapriltag.so 
    ${OPENCV_INSTALL_DIR}/lib/libopencv_core.so
    ${OPENCV_INSTALL_DIR}/lib/libopencv_imgproc.so
    ${OPENCV_INSTALL_DIR}/lib/libopencv_highgui.so
    ${OPENCV_INSTALL_DIR}/lib/libopencv_videoio.so
    ${OPENCV_INSTALL_DIR}/lib/libopencv_imgcodecs.so
    glog::glog)

# Add executable for visualize
add_executable(visualize visualize.cu)
target_link_libraries(visualize
    apriltag_cuda
    ${APRILTAG_INSTALL_DIR}/lib/libapriltag.so
    ${OPENCV_INSTALL_DIR}/lib/libopencv_core.so
    ${OPENCV_INSTALL_DIR}/lib/libopencv_imgproc.so
    ${OPENCV_INSTALL_DIR}/lib/libopencv_highgui.so
    ${OPENCV_INSTALL_DIR}/lib/libopencv_videoio.so
    ${OPENCV_INSTALL_DIR}/lib/libopencv_imgcodecs.so
    glog::glog)

# Add a network tables example.
add_executable(ntexample ntexample.cpp)
target_link_libraries(ntexample
    ${WPILIB_INSTALL_DIR}/lib/libntcore.so
    ${WPILIB_INSTALL_DIR}/lib/libwpiutil.so)

# Add the httpserver.
add_executable(http_server http_server.cpp)

# Add the test executable
add_executable(gpu_detector_test gpu_detector_test.cu)
target_link_libraries(gpu_detector_test
    apriltag_cuda
    ${APRILTAG_INSTALL_DIR}/lib/libapriltag.so
    ${OPENCV_INSTALL_DIR}/lib/libopencv_core.so
    ${OPENCV_INSTALL_DIR}/lib/libopencv_imgproc.so
    ${OPENCV_INSTALL_DIR}/lib/libopencv_highgui.so
    ${OPENCV_INSTALL_DIR}/lib/libopencv_videoio.so
    ${OPENCV_INSTALL_DIR}/lib/libopencv_imgcodecs.so
    glog::glog
    GTest::GTest)

add_executable(uwebsockets_server uwebsockets_server.cpp)
#target_link_directories(uwebsockets_server PRIVATE ${USOCKETSLIB}/uSockets/src/)
target_link_libraries(uwebsockets_server ${USOCKETSLIB} ZLIB::ZLIB)

#add_executable(uwebsockets_client uwebsockets_client.cpp)

# Add a custom target to format all source files
add_custom_target(format_all
	COMMAND clang-format -i -style=Google ${CMAKE_CURRENT_SOURCE_DIR}/*.cu ${CMAKE_CURRENT_SOURCE_DIR}/*.h ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    COMMENT "Running clang-format on all source files"
)
