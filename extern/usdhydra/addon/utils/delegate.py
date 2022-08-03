# SPDX-License-Identifier: Apache-2.0
# Copyright 2011-2022 Blender Foundation

# <pep8 compliant>

import zipfile
from concurrent import futures
from time import sleep


from . import update_ui, DELEGATES_DIR


_render_delegates = {'HdStormRendererPlugin': 'GL', 'HdRprPlugin': 'RPR'}  # remove after implementation UsdImagingGL


def get_delegates():
    # _render_delegates = {name: UsdImagingGL.Engine.GetRendererDisplayName(name)
    #                      for name in UsdImagingGL.Engine.GetRendererPlugins()}

    # if not os.path.isdir(os.environ.get('RMANTREE', "")):
    #     _render_delegates.pop('HdPrmanLoaderRendererPlugin', None)

    return _render_delegates


class Manager:
    def __init__(self):
        self.delegate_executor = None
        self.progress = None
        self.filepath = ""

    @property
    def is_available(self):
        if manager.progress is None:
            return True

        if manager.progress >= 0:
            return False

    def set_progress(self, msg):
        self.progress = msg
        update_ui(area_type='PREFERENCES')

    def build(self):
        with zipfile.ZipFile(self.filepath) as z:
            z.extractall(path=DELEGATES_DIR)

        # building process
        for i in range(101):
            sleep(0.05)
            self.set_progress(i)

        self.set_progress(None)

    def install_delegate(self):
        def delegate_build():
            try:
                self.build()

            except Exception as err:
                #log.error(err)
                self.set_progress(None)

        if not self.delegate_executor:
            self.delegate_executor = futures.ThreadPoolExecutor()

        self.delegate_executor.submit(delegate_build)


manager = Manager()
