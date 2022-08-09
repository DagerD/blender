# SPDX-License-Identifier: Apache-2.0
# Copyright 2011-2022 Blender Foundation

# <pep8 compliant>

import bpy


class USDHydra_Operator(bpy.types.Operator):
    COMPAT_ENGINES = {'USDHydra'}

    @classmethod
    def poll(cls, context):
        return context.engine in cls.COMPAT_ENGINES


class USDHydra_Panel(bpy.types.Panel):
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = 'render'
    COMPAT_ENGINES = {'USDHydra'}

    @classmethod
    def poll(cls, context):
        return context.engine in cls.COMPAT_ENGINES


class USDHydra_ChildPanel(bpy.types.Panel):
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_parent_id = ''


from . import (
    panels,
    render,
    hdrpr_render,
    light,
    material,
    world,
    object,
    usd_nodes,
    usd_stage,
)


register_classes, unregister_classes = bpy.utils.register_classes_factory([
    render.USDHYDRA_OP_data_source,
    render.USDHYDRA_OP_nodetree_camera,
    render.USDHYDRA_MT_data_source_final,
    render.USDHYDRA_MT_nodetree_camera_final,
    render.USDHYDRA_MT_nodetree_camera_viewport,
    render.USDHYDRA_MT_data_source_viewport,
    render.USDHYDRA_RENDER_PT_render_settings_final,
    render.USDHYDRA_RENDER_PT_render_settings_viewport,

    # hdrpr_render.USDHYDRA_RENDER_PT_hdrpr_settings_final,
    # hdrpr_render.USDHYDRA_RENDER_PT_hdrpr_settings_samples_final,
    # hdrpr_render.USDHYDRA_RENDER_PT_hdrpr_settings_quality_final,
    # hdrpr_render.USDHYDRA_RENDER_PT_hdrpr_settings_denoise_final,
    # hdrpr_render.USDHYDRA_RENDER_PT_hdrpr_settings_film_final,
    # hdrpr_render.USDHYDRA_RENDER_PT_hdrpr_settings_viewport,
    # hdrpr_render.USDHYDRA_RENDER_PT_hdrpr_settings_samples_viewport,
    # hdrpr_render.USDHYDRA_RENDER_PT_hdrpr_settings_quality_viewport,
    # hdrpr_render.USDHYDRA_RENDER_PT_hdrpr_settings_denoise_viewport,

    light.USDHYDRA_LIGHT_PT_light,

    material.USDHYDRA_MATERIAL_PT_context,
    material.USDHYDRA_MATERIAL_PT_preview,
    material.USDHYDRA_MATERIAL_OP_new_mx_node_tree,
    material.USDHYDRA_MATERIAL_OP_duplicate_mx_node_tree,
    material.USDHYDRA_MATERIAL_OP_convert_shader_to_mx,
    material.USDHYDRA_MATERIAL_OP_duplicate_mat_mx_node_tree,
    material.USDHYDRA_MATERIAL_OP_link_mx_node_tree,
    material.USDHYDRA_MATERIAL_OP_unlink_mx_node_tree,
    material.USDHYDRA_MATERIAL_MT_mx_node_tree,
    material.USDHYDRA_MATERIAL_PT_material,
    material.USDHYDRA_MATERIAL_PT_material_settings_surface,
    material.USDHYDRA_MATERIAL_OP_link_mx_node,
    material.USDHYDRA_MATERIAL_OP_invoke_popup_input_nodes,
    material.USDHYDRA_MATERIAL_OP_invoke_popup_shader_nodes,
    material.USDHYDRA_MATERIAL_OP_remove_node,
    material.USDHYDRA_MATERIAL_OP_disconnect_node,
    material.USDHYDRA_MATERIAL_PT_material_settings_displacement,
    material.USDHYDRA_MATERIAL_PT_output_surface,
    material.USDHYDRA_MATERIAL_PT_output_displacement,
    material.USDHYDRA_MATERIAL_PT_output_volume,
    material.USDHYDRA_MATERIAL_OP_export_mx_file,
    material.USDHYDRA_MATERIAL_OP_export_mx_console,
    material.USDHYDRA_MATERIAL_PT_tools,
    material.USDHYDRA_MATERIAL_PT_dev,

    world.USDHYDRA_WORLD_PT_surface,

    object.USDHYDRA_OBJECT_PT_usd_settings,
    object.USDHYDRA_OP_usd_object_show_hide,

    usd_nodes.USDHYDRA_NODE_PT_usd_stage,
    usd_nodes.USDHYDRA_OP_usd_tree_node_print_stage,
    usd_nodes.USDHYDRA_OP_usd_tree_node_print_root_layer,
    usd_nodes.USDHYDRA_NODE_PT_usd_nodetree_tools,
    usd_nodes.USDHYDRA_NODE_PT_usd_nodetree_dev,

    usd_stage.USDHYDRA_OP_usd_stage_prim_expand,
    # usd_stage.USDHYDRA_OP_usd_stage_prim_show_hide,
    usd_stage.USDHYDRA_UL_usd_stage,
])


def register():
    panels.register()
    register_classes()


def unregister():
    panels.unregister()
    unregister_classes()
