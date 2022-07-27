/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2011-2022 Blender Foundation */

#pragma once

#include <Python.h>

#include "MEM_guardedalloc.h"
#include "RNA_blender_cpp.h"

namespace usdhydra {

void update(BL::Context b_context, int stageId);

PyObject *addPythonSubmodule_usd_collection(PyObject *mod);

} // namespace usdhydra
