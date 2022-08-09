# SPDX-License-Identifier: Apache-2.0
# Copyright 2011-2022 Blender Foundation

# <pep8 compliant>
import platform
from pathlib import Path

import bpy


OS = platform.system()
IS_WIN = OS == 'Windows'
IS_MAC = OS == 'Darwin'
IS_LINUX = OS == 'Linux'
BLENDER_VERSION = f'{bpy.app.version[0]}.{bpy.app.version[1]}'


def title_str(str):
    s = str.replace('_', ' ')
    return s[:1].upper() + s[1:]


def code_str(str):
    return str.replace(' ', '_').replace('.', '_')


def pass_node_reroute(link):
    while isinstance(link.from_node, bpy.types.NodeReroute):
        if not link.from_node.inputs[0].links:
            return None

        link = link.from_node.inputs[0].links[0]

    return link if link.is_valid else None


def update_ui(area_type='PROPERTIES', region_type='WINDOW'):
    for window in bpy.context.window_manager.windows:
        for area in window.screen.areas:
            if area.type == area_type:
                for region in area.regions:
                    if region.type == region_type:
                        region.tag_redraw()
