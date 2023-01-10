/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2011-2022 Blender Foundation */

#include <iostream>

#include "BKE_mesh.h"
#include "BKE_mesh_runtime.h"
#include "BKE_object.h"
#include "BKE_customdata.h"

#include <BLI_span.hh>

#include "mesh.h"

using namespace pxr;

namespace usdhydra {

MeshExport::MeshExport(BL::Mesh &b_mesh)
  : mesh((Mesh *)b_mesh.ptr.data)
{
}

VtIntArray MeshExport::faceVertexCounts()
{
  int trisLen = BKE_mesh_runtime_looptri_len(mesh);
  VtIntArray ret(trisLen);
  for (int i = 0; i < trisLen; ++i) {
    ret[i] = 3; 
  }
  return ret;
}

VtIntArray MeshExport::faceVertexIndices()
{
  VtIntArray ret;
  blender::Span<MLoopTri> loopTris = mesh->looptris();
  blender::Span<MLoop> loops = mesh->loops();
  for (MLoopTri lt : loopTris) {
    ret.push_back(loops[lt.tri[0]].v);
    ret.push_back(loops[lt.tri[1]].v);
    ret.push_back(loops[lt.tri[2]].v);
  }
  return ret;
}

VtVec3fArray MeshExport::vertices()
{
  VtVec3fArray ret;
  ret.reserve(mesh->totvert);
  blender::Span<blender::float3> verts = mesh->vert_positions();
  for (blender::float3 v : verts) {
    ret.push_back(GfVec3f(v.x, v.y, v.z));
  }
  return ret;
}

VtVec3fArray MeshExport::normals()
{
  BKE_mesh_calc_normals_split(mesh);
  const float(*lnors)[3] = (float(*)[3])CustomData_get_layer(&mesh->ldata, CD_NORMAL);
  blender::Span<MLoopTri> loopTris = mesh->looptris();

  VtVec3fArray ret;
  if (lnors) {
    for (MLoopTri lt : loopTris) {
      ret.push_back(pxr::GfVec3f(lnors[lt.tri[0]]));
      ret.push_back(pxr::GfVec3f(lnors[lt.tri[1]]));
      ret.push_back(pxr::GfVec3f(lnors[lt.tri[2]]));
    }
  }
  return ret;
}

} // namespace usdhydra
