# SPDX-License-Identifier: Apache-2.0
# Copyright 2011-2022 Blender Foundation

# <pep8 compliant>

from pathlib import Path
import sys
import zipfile

import bpy
from bpy.types import AddonPreferences, Operator
from bpy.props import StringProperty, BoolProperty, EnumProperty
from bpy_extras.io_utils import ImportHelper

import _usdhydra

from ..utils import logging
log = logging.Log('preferences')


class USDHYDRA_ADDON_OP_install_delegate(Operator, ImportHelper):
    bl_idname = "usdhydra.install_render_delegate"
    bl_label = "Install Render Delegate"

    filename_ext = ".zip"
    filepath: bpy.props.StringProperty(
        name="File Path",
        maxlen=1024, subtype="FILE_PATH"
    )
    filter_glob: bpy.props.StringProperty(default="*.zip")

    def execute(self, context):
        pref = get_addon_pref()

        with zipfile.ZipFile(self.filepath) as z:
            z.extractall(path=pref.delegates_dir)

        return {'FINISHED'}


class USDHYDRA_ADDON_PT_preferences(AddonPreferences):
    bl_idname = "usdhydra"

    DEFAULT_DELEGATES_DIR = Path(bpy.utils.script_path_user()) / "addons/usdhydra/delegates"

    def init(self):
        self.update_log_level(None)
        self.update_delegates_dir(None)

    def save(self):
        if hasattr(bpy.context, 'scene'):
            bpy.ops.wm.save_userpref()

    # def update_temp_dir(self, value):
    #     if not Path(self.tmp_dir).exists() or tempfile.gettempdir() == str(Path(self.tmp_dir)):
    #         log.info(f"Current temp directory is {tempfile.gettempdir()}")
    #         return
    #
    #     tempfile.tempdir = Path(self.tmp_dir)
    #     bpy.context.preferences.addons['usdhydra'].preferences['tmp_dir'] = str(_usdhydra.utils.get_temp_dir())
    #     log.info(f"Current temp directory is changed to {bpy.context.preferences.addons['usdhydra'].preferences.tmp_dir}")

    def update_log_level(self, context):
        logging.logger.setLevel(self.log_level)
        self.save()

    def update_delegates_dir(self, context):
        p = Path(self.delegates_dir)
        if not p.is_absolute() or p.is_file():
            log.error("Incorrect delegates folder")
            return

        p.mkdir(parents=True, exist_ok=True)

        sys.path.append(str(p / "lib/python"))
        _usdhydra.init(self.delegates_dir)
        self.save()

    # tmp_dir: StringProperty(
    #     name="Temp Directory",
    #     description="Set temp directory",
    #     maxlen=1024,
    #     subtype='DIR_PATH',
    #     default=_usdhydra.utils.get_temp_dir(),
    #     update=update_temp_dir,
    # )
    delegates_dir: StringProperty(
        name="Delegate Directory",
        description="Set delegate directory",
        maxlen=1024,
        subtype='DIR_PATH',
        default=str(DEFAULT_DELEGATES_DIR),
        update=update_delegates_dir,
    )
    dev_tools: BoolProperty(
        name="Developer Tools",
        description="Enable developer tools",
        default=False,
    )
    log_level: EnumProperty(
        name="Log Level",
        description="Select logging level",
        items=(('DEBUG', "Debug", "Log level DEBUG"),
               ('INFO', "Info", "Log level INFO"),
               ('WARNING', "Warning", "Log level WARN"),
               ('ERROR', "Error", "Log level ERROR"),
               ('CRITICAL', "Critical", "Log level CRITICAL")),
        default='INFO',
        update=update_log_level,
    )
    settings: EnumProperty(
        name="",
        items=(('SETTINGS', "Settings", "Developer settings"),
               ('DELEGATE', "Delegates", "Render delegates settings")),
        default='SETTINGS',
    )

    def draw(self, context):
        def _draw_settings(layout):
            layout.separator()
            # layout.prop(self, "tmp_dir", icon='NONE' if Path(self.tmp_dir).exists() else 'ERROR')
            layout.prop(self, "dev_tools")
            layout.prop(self, "log_level")
            layout.separator()

        def _draw_delegates(layout):
            row = layout.row()
            split = row.split(factor=0.8)
            split.prop(self, "delegates_dir")
            split.operator(USDHYDRA_ADDON_OP_install_delegate.bl_idname, icon='IMPORT', text="Install")

            layout.separator()
            for delegate in _usdhydra.session.get_render_plugins():
                row = layout.box().row(align=True)
                row.alignment = 'LEFT'
                split = row.split(factor=0.5)
                split1 = split.split(factor=0.7)
                split1.label(text=delegate['id'])
                split1.label(text=delegate['name'])
                split.label(text=delegate['path'])

            layout.separator()

        layout = self.layout
        col = layout.column()
        row = col.row(align=True)
        row.prop(self, 'settings', expand=True, text=" ")

        layout = layout.column()
        if self.settings == 'SETTINGS':
            _draw_settings(layout)

        if self.settings == 'DELEGATE':
            _draw_delegates(layout)

        # row = col.row()
        #row.operator("wm.url_open", text="Main Site", icon='URL').url = bl_info["main_web"]
        #row.operator("wm.url_open", text="Community", icon='COMMUNITY').url = bl_info["community"]


def get_addon_pref():
    return bpy.context.preferences.addons['usdhydra'].preferences
