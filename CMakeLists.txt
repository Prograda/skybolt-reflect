cmake_minimum_required (VERSION 3.26.0)

project(SkyboltReflect)

include(CTest)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#-------------------------------------------------------------
# Compiler flags
#-------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if ( MSVC )
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /we4715") # Treat missing return as error
endif()

#-------------------------------------------------------------
# Build tree
#-------------------------------------------------------------
add_subdirectory(src/SkyboltReflect)

if (BUILD_TESTING)
	add_subdirectory(src/SkyboltReflectTests)
endif()

# Install conan files (required by 'conan editable' feature. See https://docs.conan.io/en/latest/developing_packages/editable_packages.html)
if (EXISTS ${CMAKE_SOURCE_DIR}/conanfile.py) # File might not exist during conan build. See https://github.com/conan-io/conan/issues/5415.
	install(FILES ${CMAKE_SOURCE_DIR}/conanfile.py DESTINATION .)
endif()

# Install headers
install(
	DIRECTORY ${CMAKE_SOURCE_DIR}/src/
	DESTINATION include
	FILES_MATCHING
	PATTERN "*.h*"
)
