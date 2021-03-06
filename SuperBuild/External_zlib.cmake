

# The Silo external project for ParaView
set(zlib_source "${CMAKE_CURRENT_BINARY_DIR}/zlib")
set(zlib_build "${CMAKE_CURRENT_BINARY_DIR}/zlib-build")
set(zlib_install "${CMAKE_CURRENT_BINARY_DIR}/zlib-install")

# If Windows we use CMake otherwise ./configure
if(WIN32)

  ExternalProject_Add(zlib
    DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
    SOURCE_DIR ${zlib_source}
    BINARY_DIR ${zlib_build}
    INSTALL_DIR ${zlib_install}
    URL ${ZLIB_URL}/${ZLIB_GZ}
    URL_MD5 ${ZLIB_MD5}
    PATCH_COMMAND ${CMAKE_COMMAND} -E remove <SOURCE_DIR>/zconf.h
    CMAKE_CACHE_ARGS
      -DCMAKE_CXX_FLAGS:STRING=${pv_tpl_cxx_flags}
      -DCMAKE_C_FLAGS:STRING=${pv_tpl_c_flags}
      -DCMAKE_BUILD_TYPE:STRING=${CMAKE_CFG_INTDIR}
      ${pv_tpl_compiler_args}
      ${zlib_EXTRA_ARGS}
    CMAKE_ARGS
      -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  )
  
  # the zlib library should be named zlib1.lib not zlib.lib
  ExternalProject_Add_Step(zlib RenameLib
    COMMAND ${CMAKE_COMMAND} -E copy ${zlib_install}/lib/zlib${_LINK_LIBRARY_SUFFIX} ${zlib_install}/lib/zlib1${_LINK_LIBRARY_SUFFIX}
    DEPENDEES install
    )

else()
  ExternalProject_Add(zlib
    DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
    SOURCE_DIR ${zlib_source}
    INSTALL_DIR ${zlib_install}
    URL ${ZLIB_URL}/${ZLIB_GZ}
    URL_MD5 ${ZLIB_MD5}
    PATCH_COMMAND ${CMAKE_COMMAND} -E remove <SOURCE_DIR>/zconf.h
    BUILD_IN_SOURCE 1
    PATCH_COMMAND ""
    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
  )

endif()

set(ZLIB_INCLUDE_DIR ${zlib_install}/include)

if(WIN32)
  set(ZLIB_LIBRARY "${zlib_install}/lib/zlib1${_LINK_LIBRARY_SUFFIX}")
else()
  set(ZLIB_LIBRARY ${zlib_install}/lib/libz${_LINK_LIBRARY_SUFFIX})
endif()
