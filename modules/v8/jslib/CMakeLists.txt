cmake_minimum_required(VERSION 3.0)
project(JavaScriptWrapper)


option(JWRAPPER_ENGINE_SM  "USE Spider Monkey javascript engine" False)
option(JWRAPPER_ENGINE_JSC  "USE JavascriptCore javascript engine" False)
include_directories("D:\\Aristocrat\\POCs\\Godot\\JavaScript\\sanajay-godot")
include_directories("D:\\Aristocrat\\POCs\\Godot\\JavaScript\\sanajay-godot\\platform\\windows")
set(JWRAPPER_SOURCES 
	"${CMAKE_CURRENT_LIST_DIR}/JavaScriptWrapper.cpp"
	
	"${CMAKE_CURRENT_LIST_DIR}/js_script_instance.h"
	"${CMAKE_CURRENT_LIST_DIR}/js_language.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/js_language.h"
	"${CMAKE_CURRENT_LIST_DIR}/js_script.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/js_script.h"
	"${CMAKE_CURRENT_LIST_DIR}/js_script_instance.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/js_registrations.h"
	"${CMAKE_CURRENT_LIST_DIR}/js_registrations.cpp"
)
include(v8_test.cmake)
include(chakra_test.cmake)