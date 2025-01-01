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
find_package(Threads REQUIRED)

#set(GLOG_INSTALL_DIR ${CMAKE_BINARY_DIR}/glog-install)
#set(GTEST_INSTALL_DIR ${CMAKE_BINARY_DIR}/gtest-install)
set(OPENCV_INSTALL_DIR ${CMAKE_BINARY_DIR}/OpenCV-install)
set(WPILIB_INSTALL_DIR ${CMAKE_BINARY_DIR}/wpilib-install)
set(CCCL_INSTALL_DIR ${CMAKE_BINARY_DIR}/cccl-install)
set(SEASOCKS_INSTALL_DIR ${CMAKE_BINARY_DIR}/seasocks-install)
set(JSON_INSTALL_DIR ${CMAKE_BINARY_DIR}/json-install)
set(APRILTAG_INSTALL_DIR ${CMAKE_BINARY_DIR}/apriltag-install)

# Determine number of processors for the parallel builds.
if(NOT DEFINED NUM_PROCESSORS)
    ProcessorCount(NUM_PROCESSORS)
    if(NUM_PROCESSORS EQUAL 0)
        set(NUM_PROCESSORS 1)
    endif()
endif()

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

# Add seasocks package
ExternalProject_Add(
    seasocks
    PREFIX ${CMAKE_BINARY_DIR}/seasocks
    GIT_REPOSITORY https://github.com/mattgodbolt/seasocks.git
    GIT_TAG v1.4.6
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${SEASOCKS_INSTALL_DIR}
    BUILD_COMMAND make -j${NUM_PROCESSORS}
    INSTALL_COMMAND make install
)

ExternalProject_Add(
    json
    PREFIX ${CMAKE_BINARY_DIR}/json
    GIT_REPOSITORY https://github.com/nlohmann/json
    GIT_TAG v3.11.3
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${JSON_INSTALL_DIR} -DJSON_BuildTests=OFF
    BUILD_COMMAND make -j${NUM_PROCESSORS}
    INSTALL_COMMAND make install
)

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
add_dependencies(apriltag OpenCV wpilib CCCL seasocks json)

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
    src/apriltag_detect.cu 
    src/apriltag_gpu.cu
    src/cuda_frc971.cu
    src/labeling_allegretti_2019_BKE.cu
    src/line_fit_filter.cu
    src/points.cu
    src/threshold.cu
    src/apriltag_utils.cu
    src/DoubleArraySender.cpp
    src/DoubleValueSender.cpp
    src/IntegerValueSender.cpp
    src/BooleanValueSender.cpp
    src/IntegerArraySender.cpp
    src/NetworkTablesUtil.cpp
    src/video_processor.cu)

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
    ${SEASOCKS_INSTALL_DIR}/include
    ${JSON_INSTALL_DIR}/include)

# Add executable for OpenCV CUDA demo
add_executable(opencv_cuda_demo src/opencv_cuda_demo.cu)
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
add_executable(visualize src/visualize.cu)
target_link_libraries(visualize
    apriltag_cuda
    ${APRILTAG_INSTALL_DIR}/lib/libapriltag.so
    ${OPENCV_INSTALL_DIR}/lib/libopencv_core.so
    ${OPENCV_INSTALL_DIR}/lib/libopencv_imgproc.so
    ${OPENCV_INSTALL_DIR}/lib/libopencv_highgui.so
    ${OPENCV_INSTALL_DIR}/lib/libopencv_videoio.so
    ${OPENCV_INSTALL_DIR}/lib/libopencv_imgcodecs.so
    glog::glog)



# Add the test executable
add_executable(gpu_detector_test src/gpu_detector_test.cu)
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

add_executable(ws_test src/ws_test.cpp)
target_link_libraries(ws_test
    ${SEASOCKS_INSTALL_DIR}/lib/libseasocks.a
    ${OPENCV_INSTALL_DIR}/lib/libopencv_core.so
    ${OPENCV_INSTALL_DIR}/lib/libopencv_imgproc.so
    ${OPENCV_INSTALL_DIR}/lib/libopencv_highgui.so
    ${OPENCV_INSTALL_DIR}/lib/libopencv_videoio.so
    ${OPENCV_INSTALL_DIR}/lib/libopencv_imgcodecs.so
    Threads::Threads
    ZLIB::ZLIB)

add_executable(ws_server src/ws_server.cu)
target_link_libraries(ws_server
    apriltag_cuda
    ${APRILTAG_INSTALL_DIR}/lib/libapriltag.so
    ${SEASOCKS_INSTALL_DIR}/lib/libseasocks.a
    ${OPENCV_INSTALL_DIR}/lib/libopencv_core.so
    ${OPENCV_INSTALL_DIR}/lib/libopencv_imgproc.so
    ${OPENCV_INSTALL_DIR}/lib/libopencv_highgui.so
    ${OPENCV_INSTALL_DIR}/lib/libopencv_videoio.so
    ${OPENCV_INSTALL_DIR}/lib/libopencv_imgcodecs.so
    ${WPILIB_INSTALL_DIR}/lib/libntcore.so
    ${WPILIB_INSTALL_DIR}/lib/libwpiutil.so
    glog::glog
    Threads::Threads
    ZLIB::ZLIB)

add_executable(json_test src/json_test.cpp)
target_link_libraries(json_test
    apriltag_cuda
    ${APRILTAG_INSTALL_DIR}/lib/libapriltag.so
    ${SEASOCKS_INSTALL_DIR}/lib/libseasocks.a
    ${OPENCV_INSTALL_DIR}/lib/libopencv_core.so
    ${OPENCV_INSTALL_DIR}/lib/libopencv_imgproc.so
    ${OPENCV_INSTALL_DIR}/lib/libopencv_highgui.so
    ${OPENCV_INSTALL_DIR}/lib/libopencv_videoio.so
    ${OPENCV_INSTALL_DIR}/lib/libopencv_imgcodecs.so
    glog::glog
    Threads::Threads
    ZLIB::ZLIB)

# Add a custom target to format all source files
add_custom_target(format_all
	COMMAND clang-format -i -style=Google ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cu ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    COMMENT "Running clang-format on all source files"
)
