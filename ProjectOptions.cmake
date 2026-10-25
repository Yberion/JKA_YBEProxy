macro(ybeproxy_setup_options)
  # Off by default, unlike the template: this DLL patches the engine's code at runtime, and the stack protector and
  # CET prologues change function bodies. Not tested with the hooks.
  option(ybeproxy_ENABLE_HARDENING "Enable hardening" OFF)
  # Off by default, unlike the template: works with the hooks, but makes the GCC build about 40% bigger and no gain
  # was measured.
  option(ybeproxy_ENABLE_IPO "Enable IPO/LTO (Release builds only)" OFF)
  option(ybeproxy_WARNINGS_AS_ERRORS "Treat Warnings As Errors" OFF)
  option(ybeproxy_ENABLE_CLANG_TIDY "Enable clang-tidy" OFF)
  option(ybeproxy_ENABLE_CPPCHECK "Enable cpp-check analysis" OFF)
  option(ybeproxy_ENABLE_CACHE "Enable ccache" ON)
endmacro()

macro(ybeproxy_global_options)
  if(ybeproxy_ENABLE_IPO)
    include(cmake/InterproceduralOptimization.cmake)
    ybeproxy_enable_ipo()
  endif()
endmacro()

macro(ybeproxy_local_options)
  include(cmake/StandardProjectSettings.cmake)

  add_library(ybeproxy_warnings INTERFACE)
  add_library(ybeproxy_options INTERFACE)

  include(cmake/CompilerWarnings.cmake)
  ybeproxy_set_project_warnings(ybeproxy_warnings ${ybeproxy_WARNINGS_AS_ERRORS})

  if(ybeproxy_ENABLE_CACHE)
    include(cmake/Cache.cmake)
    ybeproxy_enable_cache()
  endif()

  include(cmake/StaticAnalyzers.cmake)
  if(ybeproxy_ENABLE_CLANG_TIDY)
    ybeproxy_enable_clang_tidy(ybeproxy_options ${ybeproxy_WARNINGS_AS_ERRORS})
  endif()

  if(ybeproxy_ENABLE_CPPCHECK)
    ybeproxy_enable_cppcheck(${ybeproxy_WARNINGS_AS_ERRORS} "")
  endif()

  if(ybeproxy_ENABLE_HARDENING)
    include(cmake/Hardening.cmake)
    ybeproxy_enable_hardening(ybeproxy_options)
  endif()
endmacro()
