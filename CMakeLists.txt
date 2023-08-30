cmake_minimum_required(VERSION 3.16)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/CMakeConfig.txt")
    include("${CMAKE_CURRENT_SOURCE_DIR}/CMakeConfig.txt")
endif()

# Set the project name and version
project(SdfLib VERSION 0.1)

if(CMAKE_CXX_COMPILER_ID MATCHES GNU)
    add_compile_options(-fPIC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lstdc++fs -std=c++17")
endif()


# Project options
option(SDFLIB_DEBUG_INFO "Print debug information" OFF)
option(SDFLIB_BUILD_APPS "Build executables for using the library" OFF)
option(SDFLIB_BUILD_DEBUG_APPS "Build executables for debugging purposes" OFF)


if(SDFLIB_DEBUG_INFO)
    add_compile_definitions(SDFLIB_PRINT_STATISTICS)
endif()

# Specify the c++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Get all project files
file(GLOB SOURCE_FILES src/*.cpp)
file(GLOB HEADER_FILES src/*.h)
file(GLOB PUBLIC_HEADER_FILES include/SdfLib/*.h)

file(GLOB SDF_SOURCE_FILES src/sdf/*.cpp)
file(GLOB SDF_HEADER_FILES src/sdf/*.h)

file(GLOB UTILS_SOURCE_FILES src/utils/*.cpp)
file(GLOB UTILS_HEADER_FILES src/utils/*.h)

# Add libraries
add_library(${PROJECT_NAME} STATIC  ${SOURCE_FILES} ${HEADER_FILES} ${PUBLIC_HEADER_FILES}
                                    ${SDF_SOURCE_FILES} ${SDF_HEADER_FILES}
                                    ${UTILS_SOURCE_FILES} ${UTILS_HEADER_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC include/)
target_include_directories(${PROJECT_NAME} PRIVATE src/)

# Add openmp
set(ASSIMP_WARNINGS_AS_ERRORS OFF)
set(OpenMP_C "${CMAKE_C_COMPILER}")
set(OpenMP_C_FLAGS "-Xclang -fopenmp -I/opt/homebrew/opt/libomp/include")
set(OpenMP_C_LIB_NAMES "libomp")
set(OpenMP_libomp_LIBRARY "omp")
set(OpenMP_CXX "${CMAKE_CXX_COMPILER}")
set(OpenMP_CXX_FLAGS "-Xclang -fopenmp -I/opt/homebrew/opt/libomp/include")
set(OpenMP_CXX_LIB_NAMES "libomp")
set(OpenMP_libomp_LIBRARY "omp")

# Add dependencies
add_subdirectory(libs)

target_link_libraries(${PROJECT_NAME} PUBLIC glm)
target_link_libraries(${PROJECT_NAME} PUBLIC assimp)
target_link_libraries(${PROJECT_NAME} PUBLIC args)
target_link_libraries(${PROJECT_NAME} PUBLIC spdlog)
target_link_libraries(${PROJECT_NAME} PUBLIC cereal)
target_link_libraries(${PROJECT_NAME} PUBLIC eigen)
target_link_libraries(${PROJECT_NAME} PUBLIC icg)

if(CMAKE_CXX_COMPILER_ID MATCHES GNU)
    target_link_libraries(${PROJECT_NAME} PUBLIC -lstdc++fs)
endif()

# Add openMP
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message("Enabling openmp llvm extension")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /openmp:llvm")
else()
    find_package(OpenMP)
    if(OpenMP_CXX_FOUND)
        message("OpenMP version ${OpenMP_CXX_VERSION}")
        target_link_libraries(${PROJECT_NAME} PUBLIC OpenMP::OpenMP_CXX)
        set(OpenMP_AVAILABLE True)
    else()
        set(OpenMP_AVAILABLE False)
    endif()
endif()                   

# Add executable
if (NOT UNIX)
    add_library(SdfLibUnity SHARED src/tools/SdfLibUnity/SdfExportFunc.h
                                src/tools/SdfLibUnity/SdfExportFunc.cpp)
    target_link_libraries(SdfLibUnity PRIVATE ${PROJECT_NAME})
endif()

if(SDFLIB_BUILD_APPS)
    add_executable(SdfExporter src/tools/SdfExporter/main.cpp)
    target_link_libraries(SdfExporter PUBLIC ${PROJECT_NAME})

    add_executable(SdfError src/tools/SdfError/main.cpp)
    target_link_libraries(SdfError PUBLIC ${PROJECT_NAME})
endif()

if(SDFLIB_BUILD_DEBUG_APPS)
    add_executable(TestScene src/tools/TestScene/main.cpp)
    target_link_libraries(TestScene PUBLIC ${PROJECT_NAME} glfw imgui imguizmo glad)

    add_executable(TriangleDistanceTest src/tools/TriangleDistanceTest/main.cpp)
    target_link_libraries(TriangleDistanceTest PUBLIC ${PROJECT_NAME})

    add_executable(UniformGridSdfTest src/tools/UniformGridSdfTest/main.cpp)
    target_link_libraries(UniformGridSdfTest PUBLIC ${PROJECT_NAME})

    add_executable(UniformGridSdfOctreeTest src/tools/UniformGridSdfOctreeTest/main.cpp)
    target_link_libraries(UniformGridSdfOctreeTest PUBLIC ${PROJECT_NAME})

    add_executable(SdfOctreeTest src/tools/SdfOctreeTest/main.cpp)
    target_link_libraries(SdfOctreeTest PUBLIC ${PROJECT_NAME})

    add_executable(GJKtest src/tools/GJKtest/main.cpp)
    target_link_libraries(GJKtest PUBLIC ${PROJECT_NAME})

    add_executable(CalculateInterpolationParameters src/tools/CalculateInterpolationParameters/main.cpp)
    target_link_libraries(CalculateInterpolationParameters PUBLIC ${PROJECT_NAME})

    ## Uncomment to test with the other methods
    # set(CGAL_DIR C:/Users/eduard/vcpkg/installed/x64-windows/share/cgal)
    # set(GMP_INCLUDE_DIR C:/Users/eduard/vcpkg/installed/x64-windows/include)
    # set(GMP_LIBRARIES C:/Users/eduard/vcpkg/installed/x64-windows/lib/gmp.lib)
    # set(MPFR_INCLUDE_DIR C:/Users/eduard/vcpkg/installed/x64-windows/include)
    # set(MPFR_LIBRARIES C:/Users/eduard/vcpkg/installed/x64-windows/lib/mpfr.lib)
    # set(Boost_INCLUDE_DIR C:/Users/eduard/vcpkg/installed/x64-windows/include)
    # find_package(CGAL CONFIG REQUIRED)

    # add_executable(SdfOffsets src/tools/SdfOffsets/main.cpp)
    # target_link_libraries(SdfOffsets PUBLIC ${PROJECT_NAME} CGAL::CGAL)

    # list(APPEND CMAKE_MODULE_PATH "C:/Program Files/OpenVDB/lib/cmake/OpenVDB")
    # set(BOOST_ROOT C:/Users/eduard/Documents/boost_1_76_0/stage/lib)
    # set(Boost_INCLUDE_DIR C:/Users/eduard/Documents/boost_1_76_0)
    # find_package(OpenVDB REQUIRED)

    # list(APPEND CMAKE_MODULE_PATH "C:/Program Files/OpenVDB/lib/cmake/OpenVDB")
    # #set(Tbb_INCLUDE_DIR C:/dev/vcpkg/installed/x64-windows/include/tbb)
    # #set(Tbb_LIB_COMPONENTS C:/dev/vcpkg/installed/x64-windows/bin)
    # include(C:/Users/user/vcpkg/scripts/buildsystems/vcpkg.cmake)
    # find_package(TBB CONFIG REQUIRED)
    # find_package(OpenVDB REQUIRED)

    add_executable(SdfErrorCompare src/tools/SdfErrorCompare/main.cpp)
    #target_link_libraries(SdfErrorCompare PUBLIC ${PROJECT_NAME} CGAL::CGAL OpenVDB::openvdb)
    # target_link_libraries(SdfErrorCompare PUBLIC ${PROJECT_NAME} OpenVDB::openvdb)
    target_link_libraries(SdfErrorCompare PUBLIC ${PROJECT_NAME})
endif()
