/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2011-2022 Blender Foundation */

#include <pxr/base/vt/array.h>
#include <pxr/base/gf/vec2f.h>
#include <pxr/imaging/hd/light.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/usd/usdLux/tokens.h>

#include "DNA_node_types.h"
#include "BKE_node.h"
#include "BKE_node_runtime.hh"
#include "BKE_image.h"
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

WorldData::WorldData(View3DShading *shading, World *world)
  : shading(shading),
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
        data[HdLightTokens->textureFile] = SdfAssetPath(
            "C:\\Users\\user\\Desktop\\WUH6nAqWDDk.jpg",
            "C:\\Users\\user\\Desktop\\WUH6nAqWDDk.jpg");  // get_image_filepath(color_input_node);
      }
    }

    //blender::Span<bNode *> world_nodes = world->nodetree->nodes_by_type("ShaderNodeOutputWorld");
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

string WorldData::get_image_filepath(const bNode *tex_node)
{
  if (!tex_node) {
    return "";
  }
  Image *tex_image = reinterpret_cast<Image *>(tex_node->id);
  if (!tex_image || !BKE_image_has_filepath(tex_image)) {
    return "";
  }

  if (BKE_image_has_packedfile(tex_image)) {
    /* Put image in the same directory as the .MTL file. */
    const char *filename = BLI_path_slash_rfind(tex_image->filepath) + 1;
    fprintf(stderr,
            "Packed image found:'%s'. Unpack and place the image in the same "
            "directory as the .MTL file.\n",
            filename);
    return filename;
  }

  char path[FILE_MAX];
  BLI_strncpy(path, tex_image->filepath, FILE_MAX);

  if (tex_image->source == IMA_SRC_SEQUENCE) {
    char head[FILE_MAX], tail[FILE_MAX];
    ushort numlen;
    int framenr = static_cast<NodeTexImage *>(tex_node->storage)->iuser.framenr;
    BLI_path_sequence_decode(path, head, tail, &numlen);
    BLI_path_sequence_encode(path, head, tail, numlen, framenr);
  }

  return path;
};

}  // namespace blender::render::hydra
