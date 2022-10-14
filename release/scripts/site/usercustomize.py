# SPDX-License-Identifier: GPL-2.0-or-later
# Make shared libraries needed by modules available in standalone Python binary.

import sys
import os

exe_dir, exe_file = os.path.split(sys.executable)

if sys.platform == 'win32':
    platform_delimiter = ';'
    if exe_file.startswith('python'):
        blender_dir = os.path.abspath(os.path.join(exe_dir, '..', '..', '..','blender.shared'))
        os.add_dll_directory(blender_dir)
        import_paths = os.getenv('PXR_USD_WINDOWS_DLL_PATH')
        if import_paths is None:
            os.environ["PXR_USD_WINDOWS_DLL_PATH"] = blender_dir
else:
    platform_delimiter = ':'

materialx_libs_dir = os.path.abspath(os.path.join(exe_dir, '..', '..', '..', 'blender.shared',
                                                  'materialx', 'libraries'))
materialx_libs_env = os.getenv('MATERIALX_SEARCH_PATH')
os.environ["MATERIALX_SEARCH_PATH"] = (materialx_libs_dir + platform_delimiter + materialx_libs_env) \
                                        if materialx_libs_env else materialx_libs_dir
