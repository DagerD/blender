# SPDX-License-Identifier: GPL-2.0-or-later

set(MATERIALX_EXTRA_ARGS
  -DMATERIALX_BUILD_PYTHON=ON
  -DMATERIALX_INSTALL_PYTHON=OFF
  -DMATERIALX_PYTHON_EXECUTABLE=${PYTHON_BINARY}
)

if(WIN32)
  ExternalProject_Add(external_materialx
    URL file://${PACKAGE_DIR}/${MATERIALX_FILE}
    DOWNLOAD_DIR ${DOWNLOAD_DIR}
    URL_HASH ${MATERIALX_HASH_TYPE}=${MATERIALX_HASH}
    PREFIX ${BUILD_DIR}/materialx
    #PATCH_COMMAND ${PATCH_CMD} -p 1 -d ${BUILD_DIR}/materialx/src/external_materialx < ${PATCH_DIR}/materialx.diff
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${LIBDIR}/materialx ${DEFAULT_CMAKE_FLAGS} ${MATERIALX_EXTRA_ARGS}
    INSTALL_DIR ${LIBDIR}/materialx
  )

  ExternalProject_Add_Step(external_materialx after_install
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${LIBDIR}/materialx ${HARVEST_TARGET}/materialx
    DEPENDEES install
  )

endif()
