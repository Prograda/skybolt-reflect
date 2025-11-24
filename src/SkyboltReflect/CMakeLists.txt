file(GLOB SOURCE_FILES *.cpp *.h)

include_directories("../")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(SkyboltReflect ${SOURCE_FILES})
if(BUILD_SHARED_LIBS)
	set_target_properties(SkyboltReflect PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

install(TARGETS SkyboltReflect
	EXPORT SkyboltReflectTargets
	RUNTIME_DEPENDENCY_SET SkyboltReflectDependencies
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin
)