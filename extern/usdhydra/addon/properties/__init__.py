# SPDX-License-Identifier: Apache-2.0
# Copyright 2011-2022 Blender Foundation

# <pep8 compliant>

import bpy


class USDHydraProperties(bpy.types.PropertyGroup):
    bl_type = None

    @classmethod
    def register(cls):
        cls.bl_type.usdhydra = bpy.props.PointerProperty(
            name="USDHydra properties",
            description="USDHydra properties",
            type=cls,
        )

    @classmethod
    def unregister(cls):
        del cls.bl_type.usdhydra


from . import (
    preferences,
    scene,
    object,
    usd_stage,
    material,
    window_manager,
    hdrpr_render,
)
register_classes, unregister_classes = bpy.utils.register_classes_factory((
    preferences.USDHYDRA_ADDON_OP_install_delegate,
    preferences.USDHYDRA_ADDON_PT_preferences,


    hdrpr_render.QualitySettings,
    hdrpr_render.InteractiveQualitySettings,
    hdrpr_render.ContourSettings,
    hdrpr_render.DenoiseSettings,
    hdrpr_render.RenderSettings,

    scene.FinalRenderSettings,
    scene.ViewportRenderSettings,
    scene.SceneProperties,

    object.ObjectProperties,

    material.MaterialProperties,

    usd_stage.UsdStagePrim,
    usd_stage.UsdStage,

    window_manager.WindowManagerProperties,
))


def register():
    register_classes()

    pref = preferences.get_addon_pref()
    pref.init()


def unregister():
    unregister_classes()
