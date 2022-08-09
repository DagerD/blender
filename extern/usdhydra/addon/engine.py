# SPDX-License-Identifier: Apache-2.0
# Copyright 2011-2022 Blender Foundation

# <pep8 compliant>

import sys
import os
from pathlib import Path

import bpy
import _usdhydra

from .usd_nodes import node_tree
from .utils import stages


def exit():
    _usdhydra.exit()


class USDHydraEngine(bpy.types.RenderEngine):
    bl_idname = 'USDHydra'
    bl_label = "USD Hydra Internal"
    bl_info = "USD Hydra rendering plugin"

    bl_use_preview = False              # TODO: material and light previews are temporary disabled
    bl_use_shading_nodes = True
    bl_use_shading_nodes_custom = False
    bl_use_gpu_context = True

    session = None

    def __init__(self):
        self.session = None

    def __del__(self):
        if not self.session:
            return

        session_free(self.session)

    # final render
    def update(self, data, depsgraph):
        is_blender_scene = not bool(depsgraph.scene.usdhydra.final.data_source)
        stage = 0

        if not is_blender_scene:
            usd_nodetree = node_tree.get_usd_nodetree()
            if not usd_nodetree:
                return

            output_node = usd_nodetree.output_node
            if not output_node:
                return

            stage = stages.get(output_node)
            if not stage:
                return

        if not self.session:
            self.session = session_create(self)

        session_reset(self.session, data, bpy.context, depsgraph, is_blender_scene, stage)

    def render(self, depsgraph):
        if not self.session:
            return

        session_render(self.session, depsgraph)

    def render_frame_finish(self):
        pass

    # viewport render
    def view_update(self, context, depsgraph):
        data = context.blend_data
        is_blender_scene = not bool(context.scene.usdhydra.viewport.data_source)
        stage = 0

        if not is_blender_scene:
            usd_nodetree = node_tree.get_usd_nodetree()
            if not usd_nodetree:
                return

            output_node = usd_nodetree.output_node
            if not output_node:
                return

            stage = stages.get(output_node)
            if not stage:
                return

        if not self.session:
            self.session = session_create(self)

        session_reset(self.session, data, context, depsgraph, is_blender_scene, stage)
        session_view_update(self.session, depsgraph, context, context.space_data, context.region_data)

    def view_draw(self, context, depsgraph):
        if not self.session:
            return

        session_view_draw(self.session, depsgraph, context, context.space_data, context.region_data)


def session_create(engine: USDHydraEngine):
    return _usdhydra.session.create(engine.as_pointer())


def session_free(session):
    _usdhydra.session.free(session)


def session_reset(session, data, context, depsgraph, is_blender_scene, stage):
    _usdhydra.session.reset(session, data.as_pointer(), context.as_pointer(), depsgraph.as_pointer(), is_blender_scene, stage)


def session_render(session, depsgraph):
    _usdhydra.session.render(session, depsgraph.as_pointer(), depsgraph.scene.usdhydra.final.delegate)


def session_view_draw(session, depsgraph, context, space_data, region_data):
    _usdhydra.session.view_draw(session, depsgraph.as_pointer(), context.as_pointer(),
                             space_data.as_pointer(), region_data.as_pointer())


def session_view_update(session, depsgraph, context, space_data, region_data):
    _usdhydra.session.view_update(session, depsgraph.as_pointer(), context.as_pointer(),
                             space_data.as_pointer(), region_data.as_pointer(), depsgraph.scene.usdhydra.viewport.delegate)


def session_get_render_plugins():
    return _usdhydra.session.get_render_plugins()
