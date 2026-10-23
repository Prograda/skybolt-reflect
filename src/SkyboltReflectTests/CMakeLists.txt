set(APP_NAME SkyboltReflectTests)

file(GLOB SOURCE_FILES *.cpp *.h)

include_directories("../")
find_package(Catch2 3 REQUIRED)
include(Catch)

add_executable(${APP_NAME} ${SOURCE_FILES})

target_link_libraries (${APP_NAME} PUBLIC SkyboltReflect Catch2::Catch2WithMain)

catch_discover_tests(${APP_NAME})