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
    PATCH_COMMAND ${PATCH_CMD} -p 1 -d ${BUILD_DIR}/materialx/src/external_materialx < ${PATCH_DIR}/materialx.diff
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${LIBDIR}/materialx ${DEFAULT_CMAKE_FLAGS} ${MATERIALX_EXTRA_ARGS}
    INSTALL_DIR ${LIBDIR}/materialx
  )

  ExternalProject_Add_Step(external_materialx after_install
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${LIBDIR}/materialx/include ${HARVEST_TARGET}/materialx/include
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${LIBDIR}/materialx/libraries ${HARVEST_TARGET}/materialx/libraries
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${LIBDIR}/materialx/lib/ ${HARVEST_TARGET}/materialx/lib/${BUILD_MODE}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${LIBDIR}/materialx/python/ ${HARVEST_TARGET}/materialx/python/${BUILD_MODE}
    DEPENDEES install
  )

  add_custom_command(TARGET external_materialx POST_BUILD
     COMMAND ${CMAKE_COMMAND} -E rename PyMaterialXCore_d_d.cp310-win_amd64.pyd PyMaterialXCore_d.cp310-win_amd64.pyd
     COMMAND ${CMAKE_COMMAND} -E rename PyMaterialXFormat_d_d.cp310-win_amd64.pyd PyMaterialXFormat_d.cp310-win_amd64.pyd
     COMMAND ${CMAKE_COMMAND} -E rename PyMaterialXGenGlsl_d_d.cp310-win_amd64.pyd PyMaterialXGenGlsl_d.cp310-win_amd64.pyd
     COMMAND ${CMAKE_COMMAND} -E rename PyMaterialXGenMdl_d_d.cp310-win_amd64.pyd PyMaterialXGenMdl_d.cp310-win_amd64.pyd
     COMMAND ${CMAKE_COMMAND} -E rename PyMaterialXGenOsl_d_d.cp310-win_amd64.pyd PyMaterialXGenOsl_d.cp310-win_amd64.pyd
     COMMAND ${CMAKE_COMMAND} -E rename PyMaterialXGenShader_d_d.cp310-win_amd64.pyd PyMaterialXGenShader_d.cp310-win_amd64.pyd
     COMMAND ${CMAKE_COMMAND} -E rename PyMaterialXRender_d_d.cp310-win_amd64.pyd PyMaterialXRender_d.cp310-win_amd64.pyd
     COMMAND ${CMAKE_COMMAND} -E rename PyMaterialXRenderGlsl_d_d.cp310-win_amd64.pyd PyMaterialXRenderGlsl_d.cp310-win_amd64.pyd
     COMMAND ${CMAKE_COMMAND} -E rename PyMaterialXRenderOsl_d_d.cp310-win_amd64.pyd PyMaterialXRenderOsl_d.cp310-win_amd64.pyd
     WORKING_DIRECTORY ${HARVEST_TARGET}/materialx/python/${BUILD_MODE}/MaterialX
     COMMENT "Renaming files..."
  )
endif()
