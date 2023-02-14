/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2011-2022 Blender Foundation */


#include <filesystem>

#include <pxr/base/vt/array.h>
#include <pxr/base/gf/vec2f.h>
#include <pxr/imaging/hd/light.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/usd/usdLux/tokens.h>

#include "BKE_context.h"
#include "DNA_node_types.h"
#include "DNA_windowmanager_types.h"

#include "BKE_node.h"
#include "BKE_node_runtime.hh"
#include "BKE_image.h"
#include "NOD_shader.h"

#include "world.h"
#include "../utils.h"

/* TODO : add custom tftoken "transparency"? */

using namespace pxr;
using namespace std;

namespace blender::render::hydra {

WorldData::WorldData()
  : b_context(nullptr),
    shading(nullptr),
    world(nullptr)    
{
}

WorldData::WorldData(View3DShading *shading, World *world, BL::Context *b_context)
  : b_context(b_context),
    shading(shading),
    world(world)    
{
  set_as_world();
}

std::string WorldData::name()
{
  return "";
}

int WorldData::type()
{
  //return object->type;
  return 0;
}

TfToken WorldData::prim_type()
{
  return HdPrimTypeTokens->domeLight;
}

GfMatrix4d WorldData::transform()
{
  return GfMatrix4d().SetIdentity();
}

VtValue &WorldData::get_data(TfToken const &key)
{
  return data[key];
}

bool WorldData::has_data(TfToken const &key)
{
  return data.find(key) != data.end();
}

void WorldData::update_world()
{
  set_as_world();
}

void WorldData::set_as_world()
{
  data.clear();

  data[UsdLuxTokens->orientToStageUpAxis] = true;

  if (world->use_nodes) {
    bNode *output_node = ntreeShaderOutputNode(world->nodetree, SHD_OUTPUT_ALL);
    bNodeSocket input_socket = output_node->input_by_identifier("Surface");
    bNodeLink const *link = input_socket.directly_linked_links()[0];
    bNode *input_node = link->fromnode;

    bNodeSocket color_input = input_node->input_by_identifier("Color");
    bNodeSocket strength_input = input_node->input_by_identifier("Strength");

    float const *strength = strength_input.default_value_typed<float>();
    float const *color = color_input.default_value_typed<float>();
    data[HdLightTokens->intensity] = strength[1];
    data[HdLightTokens->exposure] = 1.0f;
    data[HdLightTokens->color] = GfVec3f(color[0], color[1], color[2]);

    if (!color_input.directly_linked_links().is_empty()) {
      bNode *color_input_node = color_input.directly_linked_links()[0]->fromnode;
      if (color_input_node->type == SH_NODE_TEX_IMAGE) {
        NodeTexImage *tex = static_cast<NodeTexImage *>(color_input_node->storage);
        Image *image = (Image *)color_input_node->id;

        if (image) {
          Main *bmain = CTX_data_main((bContext *)b_context->ptr.data);
          Scene *scene = CTX_data_scene((bContext *)b_context->ptr.data);

          ReportList reports;
          ImageSaveOptions opts;
          opts.im_format.imtype = R_IMF_IMTYPE_PNG;

          string cached_image_path = cache_image(bmain, scene, image, &tex->iuser, &opts, &reports);
          if (!cached_image_path.empty()) {
            data[HdLightTokens->textureFile] = SdfAssetPath(cached_image_path, cached_image_path);
          }
        }
      }
    }
  }
  else {
    data[HdLightTokens->intensity] = 1.0f;
    data[HdLightTokens->exposure] = world->exposure;
    data[HdLightTokens->color] = GfVec3f(
        world->horr,
        world->horg,
        world->horb
    );
  }
}

void WorldData::set_as_shading()
{
  data[HdLightTokens->intensity] = 1.0f;
  data[HdLightTokens->exposure] = 1.0f;
  data[HdLightTokens->color] = GfVec3f(
      shading->single_color[0],
      shading->single_color[1],
      shading->single_color[2]
  );
}
}  // namespace blender::render::hydra
