cmake_minimum_required(VERSION 3.0)

set(JWRAPPER_ENGINE_V8 True)
set(JWRAPPER_TARGET JSV8Wrapper)
set(JWRAPPER_TARGET_LIBRARIES "")
set(JWRAPPER_TARGET_DEFINITIONS "")
set(JWRAPPER_TARGET_INCLUDES "")
set(JWRAPPER_ROOT ${CMAKE_CURRENT_LIST_DIR})
set(JWRAPPER_TARGET_COMMANDS "")



add_subdirectory(jswrapper v8)

source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${JWRAPPER_SOURCES})
add_executable(${JWRAPPER_TARGET}  
	${JWRAPPER_SOURCES}
)
#message(FATAL_ERROR ${JWRAPPER_SOURCES})
target_link_libraries(${JWRAPPER_TARGET} 
	
	${JWRAPPER_TARGET_LIBRARIES}
	"D:/Aristocrat/POCs/Godot/JavaScript/sanajay-godot/bin/godot.windows.tools.64.lib"
)

target_compile_definitions(${JWRAPPER_TARGET} PRIVATE ${JWRAPPER_TARGET_DEFINITIONS})

target_include_directories(${JWRAPPER_TARGET} PRIVATE ${JWRAPPER_TARGET_INCLUDES})


add_custom_command(TARGET ${JWRAPPER_TARGET} POST_BUILD 
	${JWRAPPER_TARGET_COMMANDS}
)