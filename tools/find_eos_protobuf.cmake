
macro( SetupProtobuf )

    if(NOT EXISTS ${protobuf_COMPILER} )
      set(protobuf_COMPILER "$ENV{HOME}/bin/protoc" CACHE FILEPATH "filepath for protobuf compiler?")
      message(FATAL_ERROR "protobuf_COMPILER must be set!")
    endif()

    if(NOT EXISTS ${protobuf_INCLUDE_DIR} )
      set(protobuf_INCLUDE_DIR "$ENV{HOME}/include" CACHE PATH "path for protobuf headers?")
      message(FATAL_ERROR "protobuf_INCLUDE_DIR must be set!")
    endif()

    if(NOT EXISTS ${protobuf_LIB} )
      set(protobuf_LIB "$ENV{HOME}/lib/libprotobuf.so" CACHE FILEPATH "path for protobuf library?")
      message(FATAL_ERROR "protobuf_LIB must be set!")
    endif()

    include_directories(${protobuf_INCLUDE_DIR})

endmacro( SetupProtobuf )
