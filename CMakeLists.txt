cmake_minimum_required(VERSION 3.16)

project(obs-fit-selected-source VERSION 0.1.0 LANGUAGES C)

include(GNUInstallDirs)

add_library(obs-fit-selected-source MODULE
  src/plugin.c
)

target_compile_features(obs-fit-selected-source PRIVATE c_std_11)
target_compile_definitions(obs-fit-selected-source PRIVATE OBS_PLUGIN_EXPORTS)
target_include_directories(obs-fit-selected-source PRIVATE src)
if(NOT WIN32)
  target_link_libraries(obs-fit-selected-source PRIVATE m)
endif()

find_package(libobs QUIET)
if(TARGET OBS::libobs)
  target_link_libraries(obs-fit-selected-source PRIVATE OBS::libobs)
else()
  find_package(PkgConfig QUIET)
  if(NOT PkgConfig_FOUND)
    message(FATAL_ERROR "libobs imported target was not found and pkg-config is unavailable")
  endif()
  pkg_check_modules(LIBOBS REQUIRED libobs)
  target_include_directories(obs-fit-selected-source PRIVATE ${LIBOBS_INCLUDE_DIRS})
  target_link_directories(obs-fit-selected-source PRIVATE ${LIBOBS_LIBRARY_DIRS})
  target_link_libraries(obs-fit-selected-source PRIVATE ${LIBOBS_LIBRARIES})
  target_compile_options(obs-fit-selected-source PRIVATE ${LIBOBS_CFLAGS_OTHER})
endif()

find_package(obs-frontend-api QUIET)
if(TARGET OBS::obs-frontend-api)
  target_link_libraries(obs-fit-selected-source PRIVATE OBS::obs-frontend-api)
elseif(TARGET OBS::frontend-api)
  target_link_libraries(obs-fit-selected-source PRIVATE OBS::frontend-api)
else()
  find_package(PkgConfig QUIET)
  if(NOT PkgConfig_FOUND)
    message(FATAL_ERROR "OBS frontend imported target was not found and pkg-config is unavailable")
  endif()
  pkg_check_modules(OBS_FRONTEND REQUIRED obs-frontend-api)
  target_include_directories(obs-fit-selected-source PRIVATE ${OBS_FRONTEND_INCLUDE_DIRS})
  target_link_directories(obs-fit-selected-source PRIVATE ${OBS_FRONTEND_LIBRARY_DIRS})
  target_link_libraries(obs-fit-selected-source PRIVATE ${OBS_FRONTEND_LIBRARIES})
  target_compile_options(obs-fit-selected-source PRIVATE ${OBS_FRONTEND_CFLAGS_OTHER})
endif()

set_target_properties(obs-fit-selected-source PROPERTIES
  PREFIX ""
)

install(TARGETS obs-fit-selected-source
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/obs-plugins"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(DIRECTORY data/
  DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/obs/obs-plugins/obs-fit-selected-source"
)
