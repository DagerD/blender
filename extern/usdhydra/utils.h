/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2011-2022 Blender Foundation */

#pragma once

#include <filesystem>
#include <string>
#include <fstream>
#include <process.h>

#include <Python.h>

#include <pxr/usd/usd/prim.h>

#include "DNA_object_types.h"

using namespace std;
using namespace pxr;

namespace usdhydra {

string get_random_string(const int len);
filesystem::path get_temp_dir(void);
filesystem::path get_temp_pid_dir(void);
string get_temp_file(string suffix, string name = "", bool is_rand = false);
string format_milliseconds(std::chrono::milliseconds secs);

const set<TfToken> SUPPORTED_PRIM_TYPES{TfToken("Xform"),
                                        TfToken("SkelRoot"),
                                        //TfToken("Scope")
                                       };

const set<TfToken> SUPPORTED_GEOM_TYPES{TfToken("Mesh"), TfToken("Camera")};

bool ignore_prim(UsdPrim usd_prim);

bool add_usd_sdf_path_prop_to_object(Object* object, const char *value);

static const char *PROP_USD_SDF_PATH_NAME = "USD SDF path";
static const int PROP_USD_SDF_PATH_MAX_LEN = 255;

PyObject *addPythonSubmodule_utils(PyObject *mod);

} // namespace usdhydra
