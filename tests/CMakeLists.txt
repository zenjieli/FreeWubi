include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.2
)
FetchContent_MakeAvailable(Catch2)

add_executable(test_engine test_engine.cpp)
target_link_libraries(test_engine PRIVATE engine_core Catch2::Catch2WithMain)
target_compile_definitions(test_engine PRIVATE TEST_DATA_DIR="${TEST_DATA_DIR}")

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(test_engine)
