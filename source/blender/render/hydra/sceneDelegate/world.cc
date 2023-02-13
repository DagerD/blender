/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2011-2022 Blender Foundation */

#include <pxr/base/vt/array.h>
#include <pxr/base/gf/vec2f.h>
#include <pxr/imaging/hd/light.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/usd/usdLux/tokens.h>

#include "DNA_node_types.h"
#include "DNA_windowmanager_types.h"
#include "BKE_context.h"
#include "BKE_node.h"
#include "BKE_node_runtime.hh"
#include "BKE_image.h"
#include "BKE_image_save.h"
#include "NOD_shader.h"
#include "BLI_path_util.h"

#include "world.h"

/* TODO : add custom tftoken "transparency"? */

using namespace pxr;
using namespace std;

namespace blender::render::hydra {

WorldData::WorldData()
  : shading(nullptr),
    world(nullptr)
{
}

WorldData::WorldData(View3DShading *shading, World *world, BL::Context *b_context)
  : shading(shading),
    world(world),
    b_context(b_context)
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
  return GfMatrix4d();
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
        Image *ima = (Image *)color_input_node->id;
        ReportList reports;
        ImageSaveOptions opts;
        Main *bmain = CTX_data_main((bContext *)b_context->ptr.data);
        if (BKE_image_save_options_init(&opts,
                                        bmain,
                                        (Scene *)b_context->scene().ptr.data,
                                        ima,
                                        &tex->iuser,
                                        true,
                                        false)) {
          STRNCPY(opts.filepath, "C:\\Users\\user\\Downloads\\test\\123_false.png");
          opts.im_format.imtype = R_IMF_IMTYPE_TIFF;
          BKE_image_save(&reports, bmain, ima, &tex->iuser, &opts);

          data[HdLightTokens->textureFile] = SdfAssetPath(
              "C:\\Users\\user\\Desktop\\WUH6nAqWDDk.jpg",
              "C:\\Users\\user\\Desktop\\WUH6nAqWDDk.jpg");
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
