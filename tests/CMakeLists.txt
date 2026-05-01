cmake_minimum_required(VERSION 3.22)  
  
find_package(GTest CONFIG REQUIRED)  
  
enable_testing()  
  
add_executable(rme_tests  
    test_position.cpp  
    test_con_vector.cpp  
)  
  
target_include_directories(rme_tests PRIVATE ${CMAKE_SOURCE_DIR}/source)  
target_link_libraries(rme_tests PRIVATE GTest::gtest GTest::gtest_main)  
target_compile_features(rme_tests PRIVATE cxx_std_23)  
  
include(GoogleTest)  
gtest_discover_tests(rme_tests)  
