option(ENABLE_PROFILING "This option will enable GNU profiling" OFF)
if (ENABLE_PROFILING)
  add_compile_definitions(GPROF=1)
  add_compile_options(-pg)
  add_link_options(-pg)
endif()

