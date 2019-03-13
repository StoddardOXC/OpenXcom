cmake_minimum_required ( VERSION 3.1 )

project ( OpenXcom )

set ( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules" )
include(GNUInstallDirs)

# For yaml-cpp
set (CMAKE_CXX_STANDARD 11)


option ( DEV_BUILD "Development Build. Disable this for release builds" ON )
option ( DUMP_CORE "Disables exception and segfault handling." OFF )
option ( BUILD_PACKAGE "Prepares build for creation of a package with CPack" ON )
option ( ENABLE_WARNING "Always show warnings (even for release builds)" OFF )
option ( FATAL_WARNING "Treat warnings as errors" OFF )
option ( ENABLE_CLANG_ANALYSIS "When building with clang, enable the static analyzer" OFF )
option ( CHECK_CCACHE "Check if ccache is installed and use it" OFF )
set ( MSVC_WARNING_LEVEL 3 CACHE STRING "Visual Studio warning levels" )
option ( FORCE_INSTALL_DATA_TO_BIN "Force installation of data to binary directory" OFF )
set ( DATADIR "" CACHE STRING "Where to place datafiles" )
set ( OPENXCOM_VERSION_STRING "" CACHE STRING "Version string (after x.x)" )

if ( CHECK_CCACHE )
  find_program( CCACHE_PROGRAM ccache )
  if( NOT CCACHE_PROGRAM )
    message ( "CCACHE requested but not found on the system." )
  else ()
    set_property( GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}" )
    set_property( GLOBAL PROPERTY RULE_LAUNCH_LINK "${CCACHE_PROGRAM}" )
    message ( "found CCACHE (${CCACHE_PROGRAM})" )
  endif()
endif ()

if ( WIN32 )
  set ( default_deps_dir "${CMAKE_SOURCE_DIR}/deps" )
endif ()

if ( APPLE )
  set ( MACOS_SDLMAIN_M_PATH "${CMAKE_SOURCE_DIR}/src/apple/SDLMain.m" CACHE STRING "Path to SDLMain.m file" )
  option ( CREATE_BUNDLE "Create a Mac OS application bundle" ON )
  if ( NOT EXISTS "${MACOS_SDLMAIN_M_PATH}" )
    message ( FATAL_ERROR "On Mac OS, SDLMain.m is required. Please set the MACOS_SDLMAIN_M_PATH variable" )
  endif ()
endif ()

if ( CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
  include(CheckCXXCompilerFlag)
  CHECK_CXX_COMPILER_FLAG("-std=gnu++11" COMPILER_SUPPORTS_CXX11)
  CHECK_CXX_COMPILER_FLAG("-std=gnu++0x" COMPILER_SUPPORTS_CXX0X)
  if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
  elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x")
  else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
  endif()
endif()

set ( DEPS_DIR "${default_deps_dir}" CACHE STRING "Dependencies directory" )

# Find OpenGL
set (OpenGL_GL_PREFERENCE LEGACY)
find_package ( OpenGL )
if ( NOT OPENGL_FOUND )
  message ( WARNING "Can't find OpenGL - continuing building without OpenGL support." )
  add_definitions(-D__NO_OPENGL)
else ()
  include_directories ( ${OPENGL_INCLUDE_DIR} )
  message ( "found openGL (${OPENGL_LIBRARIES})" )
endif ()

if(NOT UNIX AND IS_DIRECTORY ${DEPS_DIR})
   include_directories ( ${DEPS_DIR}/include/SDL ${DEPS_DIR}/include/yaml-cpp ${DEPS_DIR}/include )
   if ( CMAKE_CL_64 )
     link_directories ( ${DEPS_DIR}/lib/x64 )
   else ( )
     link_directories ( ${DEPS_DIR}/lib/Win32 )
   endif()
   set( SDL_LIBRARY SDL )
   set ( SDLGFX_LIBRARY SDL_gfx )
   set ( SDLMIXER_LIBRARY SDL_mixer )
   set ( SDLIMAGE_LIBRARY SDL_image )
   set ( YAMLCPP_LIBRARY yaml-cpp )
   set ( PKG_DEPS_LDFLAGS ${SDLIMAGE_LIBRARY} ${SDLMIXER_LIBRARY} ${SDLGFX_LIBRARY} ${SDL_LIBRARY} ${YAMLCPP_LIBRARY} ${OPENGL_LIBRARIES} )
elseif(UNIX)
  if(IS_DIRECTORY ${DEPS_DIR})
     set(ENV{PKG_CONFIG_PATH} "${DEPS_DIR}/lib/pkgconfig/")
  endif()

  include(FindPkgConfig)
  pkg_check_modules(PKG_SDL REQUIRED sdl)
  pkg_check_modules(PKG_ZLIB REQUIRED zlib)
  pkg_check_modules(PKG_SDLIMAGE REQUIRED SDL_image)
  pkg_check_modules(PKG_SDLGFX REQUIRED SDL_gfx)
  pkg_check_modules(PKG_SDLMIXER REQUIRED SDL_mixer)
  pkg_check_modules(PKG_YAMLCPP REQUIRED yaml-cpp)

  include_directories(${PKG_SDL_INCLUDE_DIRS} ${PKG_ZLIB_INCLUDE_DIRS} ${PKG_SDLIMAGE_INCLUDE_DIRS})
  include_directories(${PKG_SDLGFX_INCLUDE_DIRS} ${PKG_SDLMIXER_INCLUDE_DIRS} ${PKG_YAMLCPP_INCLUDE_DIRS})
  set(PKG_DEPS_LDFLAGS ${PKG_SDL_LDFLAGS} ${PKG_ZLIB_LDFLAGS} ${PKG_SDLIMAGE_LDFLAGS} ${PKG_SDLGFX_LDFLAGS} ${PKG_SDLMIXER_LDFLAGS} ${PKG_YAMLCPP_LDFLAGS} ${OPENGL_LIBRARIES})

  message ( STATUS "PKG_DEPS_LDFLAGS = ${PKG_DEPS_LDFLAGS} ")
else()
  message ( FATAL_ERROR "pkg-config is not supported on your platform. Use DEPS_DIR" )
endif()

# Read version number
set ( file "${CMAKE_SOURCE_DIR}/src/version.h" )
file ( READ ${file} lines )
string ( REGEX MATCH ".*OPENXCOM_VERSION_LONG \"([0-9]).([0-9])" version_line "${lines}" )
set ( CPACK_PACKAGE_VERSION_MAJOR ${CMAKE_MATCH_1} )
set ( CPACK_PACKAGE_VERSION_MINOR ${CMAKE_MATCH_2} )
set ( CPACK_PACKAGE_VERSION_PATCH "" )

if ( "${OPENXCOM_VERSION_STRING}" STREQUAL "" )
  find_package ( Git )
  if ( GIT_FOUND )
    message("git found: ${GIT_EXECUTABLE}")
    execute_process ( COMMAND ${GIT_EXECUTABLE} describe --dirty
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE git_describe_out
      ERROR_VARIABLE git_describe_error
      RESULT_VARIABLE git_describe_result
      )
    string ( REGEX MATCH "([a-z|0-9|.]*)-([0-9]*)-g([a-z|0-9]*)([-|a-z]*)" git_commit "${git_describe_out}" )
    set ( git_tag ${CMAKE_MATCH_1} )
    set ( git_nb_commit ${CMAKE_MATCH_2} )
    set ( git_commit ${CMAKE_MATCH_3} )
    set ( git_dirty ${CMAKE_MATCH_4} )
    set ( OPENXCOM_VERSION_STRING ".${git_commit}${git_dirty}" )
    execute_process ( COMMAND ${GIT_EXECUTABLE} show -s --format=%ci
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE git_show_out
      ERROR_VARIABLE git_show_error
      RESULT_VARIABLE git_show_result
    )
    string ( REGEX MATCH "[0-9-]+" git_date "${git_show_out}" )
  endif()
endif()

configure_file("${CMAKE_SOURCE_DIR}/src/git_version.h.in" "${CMAKE_CURRENT_BINARY_DIR}/git_version.h" )
include_directories ( "${CMAKE_CURRENT_BINARY_DIR}" )

# Always append the commit to version number
set ( CPACK_PACKAGE_VERSION_PATCH "${git_commit}${git_dirty}" )
set ( CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}-${CPACK_PACKAGE_VERSION_PATCH}-${git_date}" )
set ( CPACK_PACKAGE_FILE_NAME "Extended-${CPACK_PACKAGE_VERSION}-macos" )
set ( CPACK_SOURCE_PACKAGE_FILE_NAME "Extended-${CPACK_PACKAGE_VERSION}-src" )

if ( BUILD_PACKAGE )
  if ( NOT DEV_BUILD )
    string ( LENGTH "${git_dirty}" is_dirty )
    if ( ${is_dirty} GREATER 0 )
      message ( FATAL_ERROR "Release package must be built from a clean tree" )
    endif ()
    if ( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
      message ( FATAL_ERROR "Release package can't be built from a debug build" )
    endif ()
  endif ( )

  if ( NOT CPACK_GENERATOR )
    set ( CPACK_GENERATOR "TXZ" )
  endif ()
  if ( NOT CPACK_SOURCE_GENERATOR )
    set ( CPACK_SOURCE_GENERATOR "TXZ" )
  endif ()

  set ( CPACK_PACKAGE_VENDOR "The OpenXcom project" )
  set ( CPACK_PACKAGE_DESCRIPTION_SUMMARY "Open-source clone of UFO: Enemy Unknown" )
  set ( CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/cmake/modules/Description.txt" )
  set ( CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md" )
  if ( NOT APPLE )
    set ( CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.txt" )
  endif ()
  set ( CPACK_PACKAGE_CONTACT "The OpenXcom project (http://www.openxcom.org)" )

  include ( LinuxDEB )
  include ( LinuxRPM )
  include ( nsis )
  if (APPLE)
    set ( CPACK_BUNDLE_NAME "OpenXcom Extended" )
    set ( CPACK_BUNDLE_ICON "${CMAKE_SOURCE_DIR}/res/mac/AppIcon.icns" )
    set ( CPACK_BUNDLE_PLIST "${CMAKE_BINARY_DIR}/openxcom.app/Contents/Info.plist" )
    set ( CPACK_DMG_VOLUME_NAME "OpenXcom Extended" )
    set ( CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_SOURCE_DIR}/res/openxcom_block.png" )
    set ( CPACK_GENERATOR "DragNDrop")
  endif()
  include ( CPack )
  message ( "version:${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}-${CPACK_PACKAGE_VERSION_PATCH}")
endif()

if ( NOT WIN32 )
  install(FILES "${CMAKE_SOURCE_DIR}/res/linux/openxcom.desktop"
    DESTINATION "${CMAKE_INSTALL_FULL_DATAROOTDIR}/applications")
  install(FILES "${CMAKE_SOURCE_DIR}/res/linux/icons/openxcom_48x48.png"
    DESTINATION "${CMAKE_INSTALL_FULL_DATAROOTDIR}/icons/hicolor/48x48/apps" RENAME openxcom.png)
  install(FILES "${CMAKE_SOURCE_DIR}/res/linux/icons/openxcom_128x128.png"
    DESTINATION "${CMAKE_INSTALL_FULL_DATAROOTDIR}/icons/hicolor/128x128/apps" RENAME openxcom.png)
  install(FILES "${CMAKE_SOURCE_DIR}/res/linux/icons/openxcom.svg"
    DESTINATION "${CMAKE_INSTALL_FULL_DATAROOTDIR}/icons/hicolor/scalable/apps")
endif ()

add_subdirectory ( docs )
add_subdirectory ( src )
