message(STATUS "CMAKE_PREFIX_PATH = ${CMAKE_PREFIX_PATH}")

if(EXISTS ${EOS_QTDIR})
  set(ENV{QTDIR}  ${EOS_QTDIR})
endif()

if(EXISTS $ENV{QTDIR})
  list(APPEND CMAKE_MODULE_PATH $ENV{QTDIR}/lib/cmake )
  list(APPEND CMAKE_PREFIX_PATH $ENV{QTDIR}/lib/cmake )
  message(STATUS "${Yellow} CMAKE_MODULE_PATH = ${CMAKE_MODULE_PATH} ${RESET}")
  find_package(Qt5 REQUIRED
    Core
    Widgets
    Gui
    Network
    OpenGL
    HINTS $ENV{QTDIR}/lib/cmake
          $ENV{QTDIR}/lib/cmake/Qt5 )
else()
  message(WARNING  "QTDIR environment variable not set... define eosQTDIR cmake var and re-run cmake.")
  set( EOS_QTDIR "/opt/pathToQt5/lib/cmake"  CACHE PATH "where are qt5 cmake files." )
endif()

