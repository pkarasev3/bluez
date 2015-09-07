
if(WIN32)
  message(FATAL_ERROR  "windoze support not fully flushed out yet.")
endif()

string(ASCII 27 Esc)
set(Yellow "${Esc}[93m" CACHE STRING "yellow color")
set(Reset "${Esc}[0m"   CACHE STRING "reset color")

execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
if(GCC_VERSION VERSION_GREATER 4.9 OR GCC_VERSION VERSION_EQUAL 4.9)
    add_definitions( -flto -Wno-error=unused-local-typedefs )
    set(STDLIBC  "-std=c++1y")
else()
    message(WARNING  "update your compiler to be in sync with visual studio 2013 features.")
    set(STDLIBC  "-std=c++11")
endif()
message(STATUS "${Yellow}GCC_VERSION= ${GCC_VERSION}, STDLIBC=${STDLIBC} ${Reset} " )

if(NOT EXISTS ${CMAKE_BINARY_DIR}/source_shortcut)
    execute_process(COMMAND ln -s -T ${CMAKE_SOURCE_DIR}  ${CMAKE_BINARY_DIR}/source_shortcut )
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

set(POLARIS_ROOT "${PROJECT_SOURCE_DIR}")
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}")

set(CMAKE_CXX_FLAGS "")

if(CMAKE_COMPILER_IS_GNUCC)
endif()

set(CMAKE_CXX_WARNINGS "\
    -W \
    -Wall \
    -Wno-unknown-pragmas \
    -Wno-unused-parameter \
    -Wno-unused-variable \
    -Wno-unused-but-set-variable \
    ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_WARNINGS} ${STDLIBC} ")
set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} \
    -MMD \
    -fPIC -pipe \
    -g3 -Wextra -Wall \
    "
    CACHE STRING " gxx compiler flags "   )

#--coverage -ftest-coverage
set(CMAKE_CXX_FLAGS_DEBUG   " ${CMAKE_CXX_FLAGS}  -DDEBUG -D_DEBUG   " CACHE STRING "debug flags"  )
set(CMAKE_CXX_FLAGS_RELEASE " ${CMAKE_CXX_FLAGS}  -DNDEBUG -DOPT_BUILD -O3  " CACHE STRING "release flags"  )

set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_COLOR_MAKEFILE   on)

set( CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR}/config  ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake )
message(STATUS "${Yellow}cmake module path: ${CMAKE_MODULE_PATH}${Reset}")


if(WIN32)
    set(CMAKE_CXX_FLAGS " /we4288 /we4238 /we4239 /we4346\
     /wd4512 /wd4511 /wd4510  /wd4127 ${CMAKE_CXX_FLAGS} " )
endif()
