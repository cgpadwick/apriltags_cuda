include(cmake/CPM.cmake)

cmake_minimum_required(VERSION 3.16 FATAL_ERROR)  # Ensure user has a compatible version of CMake

project(ApriltagCuda LANGUAGES CUDA CXX)  # Set project name and specify the languages used

# Look for required packages.
find_package(CUDA REQUIRED)
find_package(glog REQUIRED)
find_package(GTest REQUIRED)
find_package(OpenCV 4.9.0 REQUIRED CONFIG)
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

# Add CCCL package
CPMAddPackage(
    NAME CCCL
    GITHUB_REPOSITORY nvidia/cccl
    GIT_TAG v2.3.2
    OPTIONS "CCCL_ENABLE_BENCHMARKS OFF"
)

# Add apriltag package
CPMAddPackage(
    NAME apriltag
    GITHUB_REPOSITORY cgpadwick/apriltag
    GIT_TAG 3.3.0
)

# Add pybind11 package
CPMAddPackage(
    NAME pybind11
    GITHUB_REPOSITORY pybind/pybind11
    GIT_TAG v2.12.0
)

message("CCCL: ${CCCL_SOURCE_DIR} APRILTAG: ${apriltag_SOURCE_DIR}")
message("pybind11: ${pybind11_SOURCE_DIR}")

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
            set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --std c++20 -g")
        else()
            set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --std c++20 -O3")
        endif()
    endif()
else()
    message(FATAL_ERROR "CUDA compiler not found. Please set CMAKE_CUDA_COMPILER.")
endif()

add_library(apriltag::apriltag ALIAS apriltag)

# Gather all source files in the current directory
set(CUDA_LIB_SOURCES 
    apriltag_detect.cu 
    apriltag_gpu.cu
    cuda_frc971.cu
    labeling_allegretti_2019_BKE.cu
    line_fit_filter.cu
    points.cu
    threshold.cu
    apriltag_utils.cu)

# Add a library with the above source files
add_library(apriltag_cuda ${CUDA_LIB_SOURCES})

# Include directories for the compiler
include_directories( 
    ${apriltag_SOURCE_DIR}
    ${apriltag_SOURCE_DIR}/common/
    ${CCCL_SOURCE_DIR}/cub
    ${CCCL_SOURCE_DIR}/libcudacxx/include
    ${CCCL_SOURCE_DIR}/thrust
    ${OpenCV_INCLUDE_DIRS})

# Add executable for OpenCV CUDA demo
add_executable(opencv_cuda_demo opencv_cuda_demo.cu)
target_link_libraries(opencv_cuda_demo 
    apriltag_cuda 
    apriltag::apriltag
    ${OpenCV_LIBRARIES}
    glog::glog
    GTest::GTest)

# Add executable for visualize
add_executable(visualize visualize.cu)
target_link_libraries(visualize
    apriltag_cuda 
    apriltag::apriltag
    ${OpenCV_LIBRARIES}
    glog::glog
    GTest::GTest)

# Add the test executable
add_executable(gpu_detector_test gpu_detector_test.cu)
target_link_libraries(gpu_detector_test
    apriltag_cuda
    apriltag::apriltag
    ${OpenCV_LIBRARIES}
    glog::glog
    GTest::GTest)

# Add python bindings target.
add_library(apriltag_bindings
    MODULE
    py_bindings.cpp
    apriltag_utils.cu)
target_include_directories(apriltag_bindings
    PRIVATE
    ${apriltag_SOURCE_DIR}
    ${apriltag_SOURCE_DIR}/common/
    ${pybind11_SOURCE_DIR}/include
    ${Python3_INCLUDE_DIRS})
target_link_libraries(apriltag_bindings
    PRIVATE
    apriltag::apriltag
    ${OpenCV_LIBRARIES}
    ${Python3_LIBRARIES})
set_target_properties(apriltag_bindings
    PROPERTIES PREFIX
    "${PYTHON_MODULE_PREFIX}"
    SUFFIX
    "${PYTHON_MODULE_EXTENSION}")

# Add a custom target to format all source files
add_custom_target(format_all
    COMMAND clang-format -i -style=Google ${CMAKE_CURRENT_SOURCE_DIR}/*.cu 
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    COMMENT "Running clang-format on all source files"
)
