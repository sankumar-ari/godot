cmake_minimum_required(VERSION 3.0)



set(JSWrapper_V8_SRC
"${CMAKE_CURRENT_LIST_DIR}/Utils.hpp"
"${CMAKE_CURRENT_LIST_DIR}/Class.cpp"
"${CMAKE_CURRENT_LIST_DIR}/Object.cpp"
"${CMAKE_CURRENT_LIST_DIR}/ObjectWrap.cpp"
"${CMAKE_CURRENT_LIST_DIR}/ScriptEngine.cpp"
"${CMAKE_CURRENT_LIST_DIR}/Utils.cpp"
"${CMAKE_CURRENT_LIST_DIR}/Base.h"
"${CMAKE_CURRENT_LIST_DIR}/Class.hpp"
"${CMAKE_CURRENT_LIST_DIR}/HelperMacros.h"
"${CMAKE_CURRENT_LIST_DIR}/Object.hpp"
"${CMAKE_CURRENT_LIST_DIR}/ObjectWrap.h"
"${CMAKE_CURRENT_LIST_DIR}/ScriptEngine.hpp"
"${CMAKE_CURRENT_LIST_DIR}/SeApi.h"

)

set(JSWrapper_V8_Debugger_SRC
"${CMAKE_CURRENT_LIST_DIR}/debugger/inspector_io.cc"
"${CMAKE_CURRENT_LIST_DIR}/debugger/inspector_socket.cc"
"${CMAKE_CURRENT_LIST_DIR}/debugger/inspector_socket_server.cc"
"${CMAKE_CURRENT_LIST_DIR}/debugger/node.cc"
"${CMAKE_CURRENT_LIST_DIR}/debugger/node_debug_options.cc"
"${CMAKE_CURRENT_LIST_DIR}/debugger/SHA1.cpp"
"${CMAKE_CURRENT_LIST_DIR}/debugger/util.cc"
"${CMAKE_CURRENT_LIST_DIR}/debugger/http_parser.c"
"${CMAKE_CURRENT_LIST_DIR}/debugger/base64.h"
"${CMAKE_CURRENT_LIST_DIR}/debugger/env.h"
"${CMAKE_CURRENT_LIST_DIR}/debugger/http_parser.h"
"${CMAKE_CURRENT_LIST_DIR}/debugger/inspector_agent.h"
"${CMAKE_CURRENT_LIST_DIR}/debugger/inspector_io.h"
"${CMAKE_CURRENT_LIST_DIR}/debugger/inspector_socket.h"
"${CMAKE_CURRENT_LIST_DIR}/debugger/inspector_socket_server.h"
"${CMAKE_CURRENT_LIST_DIR}/debugger/node.h"
"${CMAKE_CURRENT_LIST_DIR}/debugger/node_debug_options.h"
"${CMAKE_CURRENT_LIST_DIR}/debugger/node_mutex.h"
"${CMAKE_CURRENT_LIST_DIR}/debugger/SHA1.h"
"${CMAKE_CURRENT_LIST_DIR}/debugger/util.h"
"${CMAKE_CURRENT_LIST_DIR}/debugger/util-inl.h"
"${CMAKE_CURRENT_LIST_DIR}/debugger/v8_inspector_protocol_json.h"
"${CMAKE_CURRENT_LIST_DIR}/debugger/env.cc"
"${CMAKE_CURRENT_LIST_DIR}/debugger/inspector_agent.cc"

)

set(JWRAPPER_SOURCES ${JWRAPPER_SOURCES} 
	${JSWrapper_V8_SRC}
	${JSWrapper_V8_Debugger_SRC}
	PARENT_SCOPE
) 
set(V8_ROOT "${JWRAPPER_ROOT}/thirdparty/v8/")
set(LIBUV_ROOT "${JWRAPPER_ROOT}/thirdparty/libuv/")
set(ZLIB_ROOT "${JWRAPPER_ROOT}/thirdparty/zlib/")
set(JWRAPPER_TARGET_DEFINITIONS ${JWRAPPER_TARGET_DEFINITIONS}
	USE_V8_DEBUGGER=1 SCRIPT_ENGINE_TYPE=2
	PARENT_SCOPE
)
set(JWRAPPER_TARGET_INCLUDES ${JWRAPPER_TARGET_INCLUDES} 
	"${V8_ROOT}/include" "${LIBUV_ROOT}/include" "${ZLIB_ROOT}/include/zlib" "${JWRAPPER_ROOT}/thirdparty/utf8"
	PARENT_SCOPE
)
set(JWRAPPER_TARGET_LIBRARIES ${JWRAPPER_TARGET_LIBRARIES} 
	debug "${V8_ROOT}/lib/Debug/v8.dll.lib" optimized "${V8_ROOT}/lib/Release/v8.dll.lib"
	debug "${V8_ROOT}/lib/Debug/v8_libbase.dll.lib" optimized "${V8_ROOT}/lib/Release/v8_libbase.dll.lib"
	debug "${V8_ROOT}/lib/Debug/v8_libplatform.dll.lib" optimized "${V8_ROOT}/lib/Release/v8_libplatform.dll.lib"
	"${LIBUV_ROOT}/x64_Release_StaticLibrary_MT/libuv.lib" 
	"${ZLIB_ROOT}/lib/zlib/zlibstatic.lib" Ws2_32 psapi iphlpapi userenv 
	PARENT_SCOPE
)

set(JWRAPPER_TARGET_COMMANDS ${JWRAPPER_TARGET_COMMANDS}
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${V8_ROOT}/lib/${CMAKE_CFG_INTDIR}/v8_libplatform.dll" $<TARGET_FILE_DIR:${JWRAPPER_TARGET}>
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${V8_ROOT}/lib/${CMAKE_CFG_INTDIR}/v8.dll" $<TARGET_FILE_DIR:${JWRAPPER_TARGET}>
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${V8_ROOT}/lib/${CMAKE_CFG_INTDIR}/v8_libbase.dll" $<TARGET_FILE_DIR:${JWRAPPER_TARGET}>
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${V8_ROOT}/lib/${CMAKE_CFG_INTDIR}/icui18n.dll" $<TARGET_FILE_DIR:${JWRAPPER_TARGET}>
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${V8_ROOT}/lib/${CMAKE_CFG_INTDIR}/icuuc.dll" $<TARGET_FILE_DIR:${JWRAPPER_TARGET}>
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${V8_ROOT}/lib/${CMAKE_CFG_INTDIR}/natives_blob.bin" ${CMAKE_BINARY_DIR}
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${V8_ROOT}/lib/${CMAKE_CFG_INTDIR}/snapshot_blob.bin" ${CMAKE_BINARY_DIR}
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${V8_ROOT}/lib/${CMAKE_CFG_INTDIR}/icudtl.dat" ${CMAKE_BINARY_DIR}
	PARENT_SCOPE
)
