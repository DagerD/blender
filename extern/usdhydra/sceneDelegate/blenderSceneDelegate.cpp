/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2011-2022 Blender Foundation */

#include "glog/logging.h"

#include "blenderSceneDelegate.h"
#include "object.h"

namespace usdhydra {

BlenderSceneDelegate::BlenderSceneDelegate(HdRenderIndex* parentIndex, SdfPath const& delegateID, BL::Depsgraph &b_depsgraph)
  : HdSceneDelegate(parentIndex, delegateID)
  , b_depsgraph(b_depsgraph)
  , isPopulated(false)
{
}

std::unique_ptr<ObjectExport> BlenderSceneDelegate::objectExport(SdfPath const & id)
{
  std::string name = objects[id];
  for (BL::Object &obj : b_depsgraph.objects) {
    if (obj.name_full() == name) {
      return std::make_unique<ObjectExport>(obj, b_depsgraph);
    }
  }
  return nullptr;
}

void BlenderSceneDelegate::Populate()
{
  LOG(INFO) << "Populate " << isPopulated;

  if (isPopulated) {
    for (auto &update : b_depsgraph.updates) {
      BL::ID id = update.id();
      LOG(INFO) << "Update: " << id.name_full() << " " << update.is_updated_transform() << update.is_updated_geometry() << update.is_updated_shading();

      if (id.is_a(&RNA_Object)) {
        BL::Object &obj = (BL::Object &)id;
        std::string objName = obj.name_full();
        SdfPath objId = GetDelegateID().AppendElementString(TfMakeValidIdentifier(objName));
        
        if (objects.find(objId) == objects.end()) {
          LOG(INFO) << "Add mesh object: " << objId;
          GetRenderIndex().InsertRprim(HdPrimTypeTokens->mesh, this, objId);
          objects[objId] = objName;
          continue;
        }
        if (update.is_updated_geometry()) {
          LOG(INFO) << "Full updated: " << objId;
          GetRenderIndex().GetChangeTracker().MarkRprimDirty(objId, HdChangeTracker::AllDirty);
          continue;
        }
        if (update.is_updated_transform()) {
          LOG(INFO) << "Transform updated: " << objId;
          GetRenderIndex().GetChangeTracker().MarkRprimDirty(objId, HdChangeTracker::DirtyTransform);
        }
        continue;
      }
      
      if (id.is_a(&RNA_Collection)) {
        BL::Collection &col = (BL::Collection &)id;
        //std::cout << "Collection: " << col.name() << "\n";
        if (update.is_updated_transform() && update.is_updated_geometry()) {
          //available objects from depsgraph
          std::set<std::string> depsObjects;
          for (auto &inst : b_depsgraph.object_instances) {
            if (inst.is_instance()) {
              continue;
            }
            BL::Object obj = inst.object();
            if (obj.type() == BL::Object::type_MESH) {
              depsObjects.insert(obj.name_full());
            }
          }
          
          auto it = objects.begin();
          while (it != objects.end()) {
            if (depsObjects.find(it->second) == depsObjects.end()) {
              LOG(INFO) << "Removed: " << it->first;
              GetRenderIndex().RemoveRprim(it->first);
              objects.erase(it);
              it = objects.begin();
            }
            else {
              ++it;
            }
          }
        }
        continue;
      }

      //if (id.is_a(&RNA_Scene)) {
      //  BL::Scene &scene = (BL::Scene &)id;
      //  std::cout << "Scene: " << scene.name() << "\n";

      //}
      //else {
      //  std::cout << "Other: " << id.name() << "\n";
      //}

    }
    return;
  }

  for (auto &inst : b_depsgraph.object_instances) {
    if (inst.is_instance()) {
      continue;
    }
    
    BL::Object obj = inst.object();
    SdfPath objId = GetDelegateID().AppendElementString(obj.name_full());
    
    if (obj.type() == BL::Object::type_MESH) {
      LOG(INFO) << "Add mesh object: " << objId;

      GetRenderIndex().InsertRprim(HdPrimTypeTokens->mesh, this, objId);
      objects[objId] = obj.name_full();
      continue;
    }
    if (obj.type() == BL::Object::type_LIGHT) {
      //GetRenderIndex().InsertSprim(HdPrimTypeTokens->sphereLight, this, objId);
      continue;
    }
  }
  isPopulated = true;
}

HdMeshTopology BlenderSceneDelegate::GetMeshTopology(SdfPath const& id)
{
  LOG(INFO) << "GetMeshTopology: " << id.GetAsString();
  MeshExport meshExport = objectExport(id)->meshExport();
  return HdMeshTopology(PxOsdOpenSubdivTokens->catmullClark, HdTokens->rightHanded,
                        meshExport.faceVertexCounts(), meshExport.faceVertexIndices());
}

VtValue BlenderSceneDelegate::Get(SdfPath const& id, TfToken const& key)
{
  LOG(INFO) << "Get: " << id.GetAsString() << " [" << key.GetString() << "]";
  if (key == HdTokens->points) {
    VtVec3fArray points = objectExport(id)->meshExport().vertices();
    return VtValue(points);
  }
  if (key == HdTokens->normals) {
    VtVec3fArray normals = objectExport(id)->meshExport().normals();
    return VtValue(normals);
  }
  return VtValue();
}

HdPrimvarDescriptorVector BlenderSceneDelegate::GetPrimvarDescriptors(SdfPath const& id, HdInterpolation interpolation)
{
  LOG(INFO) << "GetPrimvarDescriptors: " << id.GetAsString() << " " << interpolation;
  HdPrimvarDescriptorVector primvars;
  if (interpolation == HdInterpolationVertex) {
    primvars.emplace_back(HdTokens->points, interpolation, HdPrimvarRoleTokens->point);
  }
  if (interpolation == HdInterpolationFaceVarying) {
    primvars.emplace_back(HdTokens->normals, interpolation, HdPrimvarRoleTokens->normal);
  }
  return primvars;
}

GfMatrix4d BlenderSceneDelegate::GetTransform(SdfPath const& id)
{
  LOG(INFO) << "GetTransform: " << id.GetAsString();
  return objectExport(id)->transform();
}

VtValue BlenderSceneDelegate::GetCameraParamValue(SdfPath const& id, TfToken const& key)
{
  LOG(INFO) << "GetCameraParamValue: " << id.GetAsString() << " [" << key.GetString() << "]";
  return VtValue();
}

VtValue BlenderSceneDelegate::GetLightParamValue(SdfPath const& id, TfToken const& key)
{
  LOG(INFO) << "GetLightParamValue: " << id.GetAsString() << " [" << key.GetString() << "]";
  return VtValue();
}

} // namespace usdhydra
