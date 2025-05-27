# Set MPFR_USE_STATIC_LIBS to ON to prefer loading static libraries

if(MPFR_USE_STATIC_LIBS)
  set(GMP_USE_STATIC_LIBS ON)
  if(WIN32)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
  else()
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
  endif()
endif()

find_package(GMP REQUIRED)

# Hints for non package-manager configs
set(include_hints /usr/local/include)
set(lib_hints /usr/local/lib)

if(DEFINED MPFR_DIR)
  list(append include_hints ${MPFR_DIR})
  list(append libs_hints ${MPFR_DIR})
endif()

find_path(MPFR_INCLUDE_DIR
  mpfr.h
  HINTS ${include_hints}
)
find_library(MPFR_LIBRARY
  mpfr
  HINTS ${lib_hints}
)

add_library(MPFR STATIC IMPORTED)
set_target_properties(MPFR PROPERTIES
  IMPORTED_LOCATION ${MPFR_LIBRARY}
  INTERFACE_LINK_LIBRARIES GMP
)
target_include_directories(MPFR INTERFACE ${MPFR_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  MPFR
  DEFAULT_MSG
  MPFR_INCLUDE_DIR MPFR_LIBRARY
)
mark_as_advanced(MPFR_INCLUDE_DIR MPFR_LIBRARY)
