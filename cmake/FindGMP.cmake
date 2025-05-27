# Set GMP_USE_STATIC_LIBS to ON to prefer loading static libraries

if(GMP_USE_STATIC_LIBS)
  if(WIN32)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
  else()
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
  endif()
endif()

# Hints for non package-manager configs
set(include_hints /usr/local/include)
set(lib_hints /usr/local/lib)

if(DEFINED GMP_DIR)
  list(append include_hints ${GMP_DIR})
  list(append libs_hints ${GMP_DIR})
endif()

find_path(GMPXX_INCLUDE_DIR
  gmpxx.h
  HINTS ${include_hints}
)
find_library(GMPXX_LIBRARY
  gmpxx
  HINTS ${lib_hints}
)
find_library(GMP_LIBRARY
  gmp
  HINTS ${lib_hints}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  GMP
  DEFAULT_MSG
  GMPXX_INCLUDE_DIR GMPXX_LIBRARY GMP_LIBRARY
)
mark_as_advanced(GMPXX_INCLUDE_DIR GMPXX_LIBRARY GMP_LIBRARY)
