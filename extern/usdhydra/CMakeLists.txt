# SPDX-License-Identifier: Apache-2.0
# Copyright 2011-2022 Blender Foundation

if(WIN32)
  add_definitions(-DNOMINMAX -DWIN32_LEAN_AND_MEAN)
endif()

# USD headers use deprecated TBB headers, silence warning.
add_definitions(-DTBB_SUPPRESS_DEPRECATED_MESSAGES=1)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DBOOST_DEBUG_PYTHON")

set(INC
  ../gflags/src
  # ../../intern
  ../../intern/glew-mx
  ../../intern/guardedalloc
  # ../../mikktspace
  ../../source/blender/makesdna
  ../../source/blender/makesrna
  ../../source/blender/blenlib
  ../../source/blender/depsgraph
  ../../source/blender/blenkernel
  ../../source/blender/io/usd
  ../../source/blender/io/common
  ../../source/blender/io/usd/intern
  ../../source/blender/editors/space_outliner
  ../../source/blender/windowmanager
  ${CMAKE_BINARY_DIR}/source/blender/makesrna/intern
)

set(INC_SYS
  ${PYTHON_INCLUDE_DIRS}
  ${USD_INCLUDE_DIRS}
  ${BOOST_INCLUDE_DIR}
  ${TBB_INCLUDE_DIR}
  ${GLOG_INCLUDE_DIRS}
  ${GFLAGS_INCLUDE_DIRS}
  ${GLEW_INCLUDE_PATH}
)

set(SRC
  python.cpp

  session.h
  session.cpp

  usd_node.h
  usd_node.cpp

  utils.h
  utils.cpp

  stage.h
  stage.cpp

  camera.h
  camera.cpp

  view_settings.h
  view_settings.cpp

  usd_collection.h
  usd_collection.cpp

  usdImagingLite/engine.cpp
  usdImagingLite/engine.h
  usdImagingLite/renderDataDelegate.cpp
  usdImagingLite/renderDataDelegate.h
  usdImagingLite/renderTask.cpp
  usdImagingLite/renderTask.h
  usdImagingLite/renderParams.h
)

set(LIB
  ${PYTHON_LINKFLAGS}
  ${PYTHON_LIBRARIES}
  ${BOOST_LIBRARIES}
)

set(ADDON_FILES
  addon/__init__.py
  addon/engine.py
  addon/handlers.py
)

set(ADDON_MX_NODES_FILES
  addon/mx_nodes/__init__.py
  addon/mx_nodes/node_tree.py
)

set(ADDON_MX_NODES_NODES_FILES
  addon/mx_nodes/nodes/__init__.py
  addon/mx_nodes/nodes/base_node.py
  addon/mx_nodes/nodes/categories.py
  addon/mx_nodes/nodes/gen_alglib_defs.py
  addon/mx_nodes/nodes/gen_pbrlib_defs.py
  addon/mx_nodes/nodes/gen_standard_surface.py
  addon/mx_nodes/nodes/gen_stdlib_defs.py
  addon/mx_nodes/nodes/gen_usd_preview_surface.py
)

set(ADDON_USD_NODES_FILES
  addon/usd_nodes/__init__.py
  addon/usd_nodes/node_tree.py
)

set(ADDON_USD_NODES_NODES_FILES
  addon/usd_nodes/nodes/__init__.py
  addon/usd_nodes/nodes/base_node.py
  addon/usd_nodes/nodes/converter.py
  addon/usd_nodes/nodes/input.py
  addon/usd_nodes/nodes/output.py
  addon/usd_nodes/nodes/transformations.py
)

set(ADDON_PROPERTIES_FILES
  addon/properties/__init__.py
  addon/properties/scene.py
  addon/properties/hdrpr_render.py
  addon/properties/material.py
  addon/properties/object.py
  addon/properties/usd_stage.py
  addon/properties/preferences.py
  addon/properties/window_manager.py
)

set(ADDON_UI_FILES
  addon/ui/__init__.py
  addon/ui/render.py
  addon/ui/hdrpr_render.py
  addon/ui/object.py
  addon/ui/world.py
  addon/ui/material.py
  addon/ui/light.py
  addon/ui/panels.py
  addon/ui/usd_nodes.py
  addon/ui/usd_stage.py
)

set(ADDON_UTILS_FILES
  addon/utils/__init__.py
  addon/utils/image.py
  addon/utils/mx.py
  addon/utils/stages.py
  addon/utils/logging.py
  addon/utils/delegate.py
)

set(USDHYDRA_INSTALL_PATH "scripts/addons/usdhydra")

add_definitions(${GFLAGS_DEFINES})
add_definitions(${GLOG_DEFINES})
add_definitions(-DGLOG_NO_ABBREVIATED_SEVERITIES)
add_definitions(-DBLENDER_GFLAGS_NAMESPACE=${GFLAGS_NAMESPACE})


blender_add_lib(extern_usdhydra "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")

add_dependencies(extern_usdhydra bf_rna)

delayed_install(${CMAKE_CURRENT_SOURCE_DIR} "${ADDON_FILES}" ${USDHYDRA_INSTALL_PATH})
delayed_install(${CMAKE_CURRENT_SOURCE_DIR} "${ADDON_UTILS_FILES}" ${USDHYDRA_INSTALL_PATH}/utils)
delayed_install(${CMAKE_CURRENT_SOURCE_DIR} "${ADDON_MX_NODES_FILES}" ${USDHYDRA_INSTALL_PATH}/mx_nodes)
delayed_install(${CMAKE_CURRENT_SOURCE_DIR} "${ADDON_MX_NODES_NODES_FILES}" ${USDHYDRA_INSTALL_PATH}/mx_nodes/nodes)
delayed_install(${CMAKE_CURRENT_SOURCE_DIR} "${ADDON_PROPERTIES_FILES}" ${USDHYDRA_INSTALL_PATH}/properties)
delayed_install(${CMAKE_CURRENT_SOURCE_DIR} "${ADDON_USD_NODES_FILES}" ${USDHYDRA_INSTALL_PATH}/usd_nodes)
delayed_install(${CMAKE_CURRENT_SOURCE_DIR} "${ADDON_USD_NODES_NODES_FILES}" ${USDHYDRA_INSTALL_PATH}/usd_nodes/nodes)
delayed_install(${CMAKE_CURRENT_SOURCE_DIR} "${ADDON_UI_FILES}" ${USDHYDRA_INSTALL_PATH}/ui)
