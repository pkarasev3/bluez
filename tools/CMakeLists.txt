cmake_minimum_required(VERSION 3.0)

project(bluez-hax)

enable_testing()

include(common.cmake)
include(find_eos_qt5.cmake)
include(find_eos_protobuf.cmake)

add_definitions(-DNO_MAIN_IN_PK_C -DBLUETOOTH_PLUGIN_BUILTIN)

add_compile_options( -static  -pthread )  # -ldbus-1

if(NOT DEFINED GLIB_INCLUDE_DIR)
    set(GLIB_INCLUDE_DIRS
           /usr/include/glib-2.0
           /usr/include/glib-2.0/glib
           /usr/lib/x86_64-linux-gnu/glib-2.0/include/
           /usr/include/dbus-1.0/
           /usr/lib/x86_64-linux-gnu/dbus-1.0/include/
       ) 
endif()

foreach( LIBN  ${GLIB_INCLUDE_DIRS} )
if(NOT EXISTS ${LIBN})
    message(FATAL_ERROR  "${LIBN} must exist ")
endif()
endforeach()

add_definitions( -DHAVE_CONFIG_H )
add_definitions( -DPLUGINDIR="${CMAKE_SOURCE_DIR}/plugins" )
include_directories(${GLIB_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/../)

file(GLOB  BZX_LIB  ../lib/*.c  ../lib/*.h)
add_library( bluezX_lib  STATIC  ${BZX_LIB}  )

file(GLOB  BZX_SRC  ../*.h
                    ../src/shared/*.c       ../src/shared/*.h
#                    ../peripheral/*.c ../peripheral/*.h
                    ../gdbus/*.c ../gdbus/*.h
                    ../btio/*.c  ../btio/*.h
    )
add_library( bluezX_src  STATIC  ${BZX_SRC}


../src/adapter.c
../src/advertising.c
../src/agent.c
../src/attrib-server.c
../src/dbus-common.c
../src/device.c
../src/eir.c
../src/error.c
../src/gatt-client.c
../src/gatt-database.c
../src/log.c
../src/main.c
../src/oui.c
#../src/plugin.c
../src/profile.c
../src/rfkill.c
../src/sdp-client.c
../src/sdpd-database.c
../src/sdpd-request.c
../src/sdpd-server.c
../src/sdpd-service.c
../src/sdp-xml.c
../src/service.c
../src/storage.c
../src/systemd.c
../src/textfile.c
../src/uuid-helper.c
../src/adapter.h
../src/advertising.h
../src/agent.h
../src/attio.h
../src/attrib-server.h
../src/builtin.h
../src/dbus-common.h
../src/device.h
../src/eir.h
../src/error.h
../src/gatt-client.h
../src/gatt-database.h
../src/hcid.h
../src/log.h
../src/oui.h
#../src/plugin.h
../src/profile.h
../src/sdp-client.h
../src/sdpd.h
../src/sdp-xml.h
../src/service.h
../src/storage.h
../src/systemd.h
../src/textfile.h
../src/uinput.h
../src/uuid-helper.h
 )

target_link_libraries( bluezX_src  ) # libglib-2.0.a )


set(tools_btgatt_SOURCES 
    ../tools/pk-btgatt-hack.c 
    ../src/uuid-helper.c )

add_compile_options( -static  -pthread )  # -ldbus-1

file(GLOB  GATTTOOL_SRC ../attrib/*.h ../attrib/*.c  ../client/display.h ../client/display.c  )
add_executable( 
  pk-btgatt-CPP  
  ${GATTTOOL_SRC}
  main.cpp
  ${tools_btgatt_SOURCES} 
  )

#set( bluezMainloop_LIB  ${CMAKE_SOURCE_DIR}/../src/libshared-mainloop.la )
#set( bluezInternal_LIB  ${CMAKE_SOURCE_DIR}/../lib/libbluetooth-internal.la )

#foreach( L   ${bluezMainloop_LIB} 
#             ${bluezInternal_LIB} )
#    if(NOT EXISTS ${L})
#        message(FATAL_ERROR  "${L} does not exist! ")
#    endif()
#endforeach()

target_link_libraries( pk-btgatt-CPP
    bluezX_lib  
    bluezX_src
    glib-2.0
    dbus-1
    dl
    rt
    tinfo
    pcre
    readline
#    libdl.a
#    librt.a
   )

    #${bluezMainloop_LIB} 
  #${bluezInternal_LIB} )


