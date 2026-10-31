file(GLOB_RECURSE YBEPROXY_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${YBEPROXY_SOURCES})

add_library(${PROJECT_NAME} SHARED ${YBEPROXY_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(${PROJECT_NAME} PRIVATE ybeproxy_options ybeproxy_warnings Zydis)

set_target_properties(
  ${PROJECT_NAME}
  PROPERTIES OUTPUT_NAME "jampgame${YBEPROXY_ARCH}"
             # no "lib" prefix on Linux
             PREFIX ""
             RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
             LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

target_compile_definitions(${PROJECT_NAME} PRIVATE _GAME $<$<NOT:$<CONFIG:Debug>>:FINAL_BUILD>)

if(MSVC)
  # The static MSVC runtime is set for every target in the top-level CMakeLists.txt: the DLL is extracted from a pk3
  # and loaded by the game, it cannot rely on a Visual C++ redistributable
  target_compile_definitions(
    ${PROJECT_NAME}
    PRIVATE NOMINMAX
            _CRT_SECURE_NO_WARNINGS
            _SCL_SECURE_NO_WARNINGS
            _CRT_NONSTDC_NO_DEPRECATE)
  target_compile_options(${PROJECT_NAME} PRIVATE /MP)
  if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    target_compile_options(${PROJECT_NAME} PRIVATE /arch:SSE2)
  endif()
else()
  target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:_DEBUG>)
  target_compile_options(${PROJECT_NAME} PRIVATE -fsigned-char $<$<CONFIG:Debug>:-ggdb>)

  if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    target_compile_options(${PROJECT_NAME} PRIVATE -msse2)
  endif()

  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # "x86 vm will crash without -mstackrealign since MMX instructions will be used no matter what and they corrupt
    # the frame pointer in VM calls" -ioquake3 Makefile
    target_compile_options(${PROJECT_NAME} PRIVATE -mstackrealign -mfpmath=sse)
    # The server may not have the C++ runtime (linuxjampded does not use it)
    target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++)
  endif()

  # Removes -rdynamic at link, which caused crashes
  set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")
  # Keep the symbols of the statically linked libstdc++ inside the .so: another libstdc++ already loaded in the
  # process (OpenJK) must not replace them
  target_link_options(${PROJECT_NAME} PRIVATE -Wl,--exclude-libs,ALL)
  target_compile_definitions(${PROJECT_NAME} PRIVATE "ARCH_STRING=\"${YBEPROXY_ARCH}\"")
endif()

# https://reproducible-builds.org/specs/source-date-epoch/
if(NOT "$ENV{SOURCE_DATE_EPOCH}" STREQUAL "")
  execute_process(
    COMMAND "date" "--date=@$ENV{SOURCE_DATE_EPOCH}" "+%b %_d %Y"
    OUTPUT_VARIABLE source_date
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
  target_compile_definitions(${PROJECT_NAME} PRIVATE "SOURCE_DATE=\"${source_date}\"")
endif()
