/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2011-2022 Blender Foundation */

#include <epoxy/gl.h>

#include <pxr/base/gf/camera.h>
#include <pxr/imaging/glf/drawTarget.h>
#include <pxr/usd/usdGeom/camera.h>

#include "BLI_math_matrix.h"

#include "glog/logging.h"

#include "viewportEngine.h"
#include "utils.h"

using namespace std;
using namespace pxr;

namespace blender::render::hydra {

struct CameraData {
  static CameraData init_from_camera(BL::Camera &b_camera, float transform[4][4], float ratio, float border[2][2]);
  static CameraData init_from_context(BL::Context &b_context);

  pxr::GfCamera export_gf(float tile[4]);

  BL::Camera::type_enum mode;
  float clip_range[2];
  float focal_length = 0.0;
  float sensor_size[2];
  float transform[4][4];
  float lens_shift[2];
  float ortho_size[2];
  tuple<float, float, int> dof_data;
};

struct ViewSettings {
  ViewSettings(BL::Context &b_context);

  int get_width();
  int get_height();

  pxr::GfCamera export_camera();

  CameraData camera_data;

  int screen_width;
  int screen_height;
  int border[2][2];
};

CameraData CameraData::init_from_camera(BL::Camera &b_camera, float transform[4][4], float ratio, float border[2][2])
{
  float pos[2] = {border[0][0], border[0][1]};
  float size[2] = {border[1][0], border[1][1]};

  CameraData data = CameraData();

  copy_m4_m4(data.transform, transform);

  data.clip_range[0] = b_camera.clip_start();
  data.clip_range[1] = b_camera.clip_end();
  data.mode = b_camera.type();

  if (b_camera.dof().use_dof()) {
    float focus_distance;
    if (!b_camera.dof().focus_object()) {
      focus_distance = b_camera.dof().focus_distance();
    } 
    else {
      float obj_pos[] = {b_camera.dof().focus_object().matrix_world()[3],
                         b_camera.dof().focus_object().matrix_world()[7],
                         b_camera.dof().focus_object().matrix_world()[11]};

      float camera_pos[] = {transform[0][3],
                            transform[1][3],
                            transform[2][3]};

      focus_distance = sqrt(pow((obj_pos[0] - camera_pos[0]), 2) + 
                            pow((obj_pos[1] - camera_pos[1]), 2) + 
                            pow((obj_pos[2] - camera_pos[2]), 2));
    }

    data.dof_data = tuple(max(focus_distance, 0.001f),
                          b_camera.dof().aperture_fstop(),
                          b_camera.dof().aperture_blades());
  }

  if (b_camera.sensor_fit() == BL::Camera::sensor_fit_VERTICAL) {
    data.lens_shift[0] = b_camera.shift_x() / ratio;
    data.lens_shift[1] = b_camera.shift_y();
  } 
  else if ((b_camera.sensor_fit() == BL::Camera::sensor_fit_HORIZONTAL)) {
    data.lens_shift[0] = b_camera.shift_x();
    data.lens_shift[1] = b_camera.shift_y() * ratio;
  } 
  else if ((b_camera.sensor_fit() == BL::Camera::sensor_fit_AUTO)) {
    if (ratio > 1.0f) {
      data.lens_shift[0] = b_camera.shift_x();
      data.lens_shift[1] = b_camera.shift_y() * ratio;
    } 
    else {
      data.lens_shift[0] = b_camera.shift_x() / ratio;
      data.lens_shift[1] = b_camera.shift_y();
    }
  } 
  else {
    data.lens_shift[0] = b_camera.shift_x();
    data.lens_shift[1] = b_camera.shift_y();
  }

  data.lens_shift[0] = data.lens_shift[0] / size[0] + (pos[0] + size[0] * 0.5 - 0.5) / size[0];
  data.lens_shift[1] = data.lens_shift[1] / size[1] + (pos[1] + size[1] * 0.5 - 0.5) / size[1];

  if (b_camera.type() == BL::Camera::type_PERSP) {
    data.focal_length = b_camera.lens();

    if (b_camera.sensor_fit() == BL::Camera::sensor_fit_VERTICAL) {
      data.sensor_size[0] = b_camera.sensor_height() * ratio;
      data.sensor_size[1] = b_camera.sensor_height();
    } 
    else if (b_camera.sensor_fit() == BL::Camera::sensor_fit_HORIZONTAL) {
      data.sensor_size[0] = b_camera.sensor_width();
      data.sensor_size[1] = b_camera.sensor_width() / ratio;
    } 
    else {
      if (ratio > 1.0f) {
        data.sensor_size[0] = b_camera.sensor_width();
        data.sensor_size[1] = b_camera.sensor_width() / ratio;
      } 
      else {
        data.sensor_size[0] = b_camera.sensor_width() * ratio;
        data.sensor_size[1] = b_camera.sensor_width();
      }
    }
    data.sensor_size[0] = data.sensor_size[0] * size[0];
    data.sensor_size[1] = data.sensor_size[1] * size[1];
  } 
  else if (b_camera.type() == BL::Camera::type_ORTHO) {
    if (b_camera.sensor_fit() == BL::Camera::sensor_fit_VERTICAL) {
      data.ortho_size[0] = b_camera.ortho_scale() * ratio;
      data.ortho_size[1] = b_camera.ortho_scale();
    } 
    else if (b_camera.sensor_fit() == BL::Camera::sensor_fit_HORIZONTAL) {
      data.ortho_size[0] = b_camera.ortho_scale();
      data.ortho_size[1] = b_camera.ortho_scale() / ratio;
    } 
    else {
      if (ratio > 1.0f) {
        data.ortho_size[0] = b_camera.ortho_scale();
        data.ortho_size[1] = b_camera.ortho_scale() / ratio;
      } 
      else {
        data.ortho_size[0] = b_camera.ortho_scale() * ratio;
        data.ortho_size[1] = b_camera.ortho_scale();
      }
    }

    data.ortho_size[0] = data.ortho_size[0] * size[0];
    data.ortho_size[1] = data.ortho_size[1] * size[1];

    data.clip_range[0] = b_camera.clip_start();
    data.clip_range[1] = b_camera.clip_end();
  } 
  else if (b_camera.type() == BL::Camera::type_PANO) {
    // TODO: Recheck parameters for PANO camera
    data.focal_length = b_camera.lens();
    if (b_camera.sensor_fit() == BL::Camera::sensor_fit_VERTICAL) {
      data.sensor_size[0] = b_camera.sensor_height() * ratio;
      data.sensor_size[1] = b_camera.sensor_height();
    } 
    else if (b_camera.sensor_fit() == BL::Camera::sensor_fit_HORIZONTAL) {
      data.sensor_size[0] = b_camera.sensor_height();
      data.sensor_size[1] = b_camera.sensor_height() / ratio;
    } 
    else {
      if (ratio > 1.0f) {
        data.sensor_size[0] = b_camera.sensor_width();
        data.sensor_size[1] = b_camera.sensor_width() / ratio;
      } 
      else {
        data.sensor_size[0] = b_camera.sensor_width() * ratio;
        data.sensor_size[1] = b_camera.sensor_width();
      }
    }
    data.sensor_size[0] = data.sensor_size[0] * size[0];
    data.sensor_size[1] = data.sensor_size[1] * size[1];
  } 
  else {
    data.focal_length = b_camera.lens();
    data.sensor_size[0] = b_camera.sensor_height() * ratio;
    data.sensor_size[1] = b_camera.sensor_height();
  }

  return data;
}

CameraData CameraData::init_from_context(BL::Context &b_context)
{
  // this constant was found experimentally, didn't find such option in
  // context.space_data or context.region_data
  float VIEWPORT_SENSOR_SIZE = 72.0;

  BL::SpaceView3D space_data = (BL::SpaceView3D)b_context.space_data();

  CameraData data;
  float ratio = (float)b_context.region().width() / (float)b_context.region().height();
  if (b_context.region_data().view_perspective() == BL::RegionView3D::view_perspective_PERSP) {
    data = CameraData();
    data.mode = BL::Camera::type_PERSP;
    data.clip_range[0] = space_data.clip_start();
    data.clip_range[1] = space_data.clip_end();
    data.lens_shift[0] = 0.0;
    data.lens_shift[1] = 0.0;
    data.focal_length = space_data.lens();

    if (ratio > 1.0) {
      data.sensor_size[0] = VIEWPORT_SENSOR_SIZE;
      data.sensor_size[1] = VIEWPORT_SENSOR_SIZE / ratio;
    }
    else {
      data.sensor_size[0] = VIEWPORT_SENSOR_SIZE * ratio;
      data.sensor_size[1] = VIEWPORT_SENSOR_SIZE;
    }

    invert_m4_m4(data.transform, (float(*)[4])b_context.region_data().view_matrix().data);
  }
  else if (b_context.region_data().view_perspective() == BL::RegionView3D::view_perspective_ORTHO) {
    data = CameraData();
    data.mode = BL::Camera::type_ORTHO;
    data.lens_shift[0] = 0.0f;
    data.lens_shift[1] = 0.0f;

    float ortho_size = b_context.region_data().view_distance() * VIEWPORT_SENSOR_SIZE / space_data.lens();
    float ortho_depth = space_data.clip_end();

    data.clip_range[0] = -ortho_depth * 0.5;
    data.clip_range[1] = ortho_depth * 0.5;

    if (ratio > 1.0f) {
      data.ortho_size[0] = ortho_size;
      data.ortho_size[1] = ortho_size / ratio;
    } else {
      data.ortho_size[0] = ortho_size * ratio;
      data.ortho_size[1] = ortho_size;
    }
    
    invert_m4_m4(data.transform, (float(*)[4])b_context.region_data().view_matrix().data);
  }
  else if (b_context.region_data().view_perspective() == BL::RegionView3D::view_perspective_CAMERA) {
    BL::Object camera_obj = space_data.camera();

    float border[2][2] = {{0, 0}, {1, 1}};
    float inverted_transform[4][4];
    invert_m4_m4(inverted_transform, (float(*)[4])b_context.region_data().view_matrix().data);    

    data = CameraData::init_from_camera((BL::Camera &)camera_obj.data(), inverted_transform, ratio, border);

    // This formula was taken from previous plugin with corresponded comment
    // See blender/intern/cycles/blender/blender_camera.cpp:blender_camera_from_view (look for 1.41421f)
    float zoom = 4.0 / pow((pow(2.0, 0.5) + b_context.region_data().view_camera_zoom() / 50.0), 2);

    // Updating lens_shift due to viewport zoom and view_camera_offset
    // view_camera_offset should be multiplied by 2
    data.lens_shift[0] = (data.lens_shift[0] + b_context.region_data().view_camera_offset()[0] * 2) / zoom;
    data.lens_shift[1] = (data.lens_shift[1] + b_context.region_data().view_camera_offset()[1] * 2) / zoom;

    if (data.mode == BL::Camera::type_ORTHO) {
      data.ortho_size[0] *= zoom;
      data.ortho_size[1] *= zoom;
    }
    else {
      data.sensor_size[0] *= zoom;
      data.sensor_size[1] *= zoom;
    }
  }

  return data;
}

pxr::GfCamera CameraData::export_gf(float tile[4])
{
  float tile_pos[2] = {tile[0], tile[1]}, tile_size[2] = {tile[2], tile[3]};

  pxr::GfCamera gf_camera = pxr::GfCamera();

  gf_camera.SetClippingRange(pxr::GfRange1f(this->clip_range[0], this->clip_range[1]));

  vector<float> lens_shift = {(float)(this->lens_shift[0] + tile_pos[0] + tile_size[0] * 0.5 - 0.5) / tile_size[0],
                              (float)(this->lens_shift[1] + tile_pos[1] + tile_size[1] * 0.5 - 0.5) / tile_size[1]};

  if (this->mode == BL::Camera::type_PERSP) {
    gf_camera.SetProjection(pxr::GfCamera::Projection::Perspective);
    gf_camera.SetFocalLength(this->focal_length);

    vector<float> sensor_size = {this->sensor_size[0] * tile_size[0], this->sensor_size[1] * tile_size[1]};

    gf_camera.SetHorizontalAperture(sensor_size[0]);
    gf_camera.SetVerticalAperture(sensor_size[1]);

    gf_camera.SetHorizontalApertureOffset(lens_shift[0] * sensor_size[0]);
    gf_camera.SetVerticalApertureOffset(lens_shift[1] * sensor_size[1]);
  }
  else if (this->mode == BL::Camera::type_ORTHO) {
    gf_camera.SetProjection(pxr::GfCamera::Projection::Orthographic);

    // Use tenths of a world unit accorging to USD docs https://graphics.pixar.com/usd/docs/api/class_gf_camera.html
    float ortho_size[2] = {this->ortho_size[0] * tile_size[0] * 10, 
                           this->ortho_size[1] * tile_size[1] * 10};

    gf_camera.SetHorizontalAperture(ortho_size[0]);
    gf_camera.SetVerticalAperture(ortho_size[1]);

    gf_camera.SetHorizontalApertureOffset(lens_shift[0] * this->ortho_size[0] * tile_size[0] * 10);
    gf_camera.SetVerticalApertureOffset(lens_shift[1] * this->ortho_size[1] * tile_size[1] * 10);
  }
  else if (this->mode == BL::Camera::type_PANO) {
    // TODO: store panoramic camera settings
  }

  double transform_d[4][4];
  for (int i = 0 ; i < 4; i++) {
    for (int j = 0 ; j < 4; j++) {
      transform_d[i][j] = (double)transform[i][j];
    }
  }
  gf_camera.SetTransform(pxr::GfMatrix4d(transform_d));
  
  return gf_camera;
}

ViewSettings::ViewSettings(BL::Context &b_context)
{
  camera_data = CameraData::init_from_context(b_context);

  screen_width = b_context.region().width();
  screen_height = b_context.region().height();

  float width_half = screen_width / 2.0f;
  float height_half = screen_height / 2.0f;

  BL::Scene b_scene = b_context.scene();

  //getting render border
  int x1 = 0, y1 = 0;
  int x2 = screen_width, y2 = screen_height;

  if (b_context.region_data().view_perspective() == BL::RegionView3D::view_perspective_CAMERA) {
    if (b_scene.render().use_border()) {
      BL::Object b_camera_obj = b_scene.camera();
      BL::Camera b_camera = (BL::Camera)b_camera_obj.data();

      float camera_points[4][3];

      b_camera.view_frame(b_scene, camera_points[0], camera_points[1], camera_points[2], camera_points[3]);

      BL::Array<float, 16> region_persp_matrix = b_context.region_data().perspective_matrix();
      BL::Array<float, 16> camera_world_matrix = b_camera_obj.matrix_world();

      float screen_points[4][2];

      for (int i = 0 ; i < 4; i++) {
        float world_location[] = {camera_points[i][0], camera_points[i][1], camera_points[i][2], 1.0f};
        mul_m4_v4((float(*)[4])camera_world_matrix.data, world_location);
        mul_m4_v4((float(*)[4])region_persp_matrix.data, world_location);

        if (world_location[3] > 0.0) {
          screen_points[i][0] = width_half + width_half * (world_location[0] / world_location[3]);
          screen_points[i][1] = height_half + height_half * (world_location[1] / world_location[3]);
        }
      }

      // getting camera view region
      float x1_f = min({screen_points[0][0], screen_points[1][0], screen_points[2][0], screen_points[3][0]});
      float x2_f = max({screen_points[0][0], screen_points[1][0], screen_points[2][0], screen_points[3][0]});
      float y1_f = min({screen_points[0][1], screen_points[1][1], screen_points[2][1], screen_points[3][1]});
      float y2_f = max({screen_points[0][1], screen_points[1][1], screen_points[2][1], screen_points[3][1]});

      // adjusting region to border
      float x = x1_f, y = y1_f;
      float dx = x2_f - x1_f, dy = y2_f - y1_f;

      x1 = x + b_scene.render().border_min_x() * dx;
      x2 = x + b_scene.render().border_max_x() * dx;
      y1 = y + b_scene.render().border_min_y() * dy;
      y2 = y + b_scene.render().border_max_y() * dy;

      // adjusting to region screen resolution
      x1 = max(min(x1, screen_width), 0);
      x2 = max(min(x2, screen_width), 0);
      y1 = max(min(y1, screen_height), 0);
      y2 = max(min(y2, screen_height), 0);
    }
  }
  else {
    if (((BL::SpaceView3D)b_context.space_data()).use_render_border()) {
      int x = x1, y = y1;
      int dx = x2 - x1, dy = y2 - y1;

      x1 = int(x + ((BL::SpaceView3D)b_context.space_data()).render_border_min_x() * dx);
      x2 = int(x + ((BL::SpaceView3D)b_context.space_data()).render_border_max_x() * dx);
      y1 = int(y + ((BL::SpaceView3D)b_context.space_data()).render_border_min_y() * dy);
      y2 = int(y + ((BL::SpaceView3D)b_context.space_data()).render_border_max_y() * dy);
    }
  }

  border[0][0] = x1;
  border[0][1] = y1;
  border[1][0] = x2 - x1;
  border[1][1] = y2 - y1;
}

int ViewSettings::get_width()
{
  return border[1][0];
}

int ViewSettings::get_height()
{
  return border[1][1];
}

GfCamera ViewSettings::export_camera()
{
  float tile[4] = {(float)border[0][0] / screen_width, (float)border[0][1] / screen_height,
                   (float)border[1][0] / screen_width, (float)border[1][1] / screen_height};
  return camera_data.export_gf(tile);
}

GLTexture::GLTexture()
  : textureId(0)
  , width(0)
  , height(0)
  , channels(4)
{
}

GLTexture::~GLTexture()
{
  if (textureId) {
    free();
  }
}

void GLTexture::setBuffer(pxr::HdRenderBuffer *buffer)
{
  if (!textureId) {
    create(buffer);
    return;
  }

  if (width != buffer->GetWidth() || height != buffer->GetHeight()) {
    free();
    create(buffer);
    return;
  }

  glBindTexture(GL_TEXTURE_2D, textureId);
    
  void *data = buffer->Map();
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_FLOAT, data);
  buffer->Unmap();
}

void GLTexture::create(pxr::HdRenderBuffer *buffer)
{
  width = buffer->GetWidth();
  height = buffer->GetHeight();
  channels = HdGetComponentCount(buffer->GetFormat());

  glGenTextures(1, &textureId);
  glBindTexture(GL_TEXTURE_2D, textureId);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  
  void *data = buffer->Map();
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, data);
  buffer->Unmap();
}

void GLTexture::free()
{
  glDeleteTextures(1, &textureId);
  textureId = 0;
}

void GLTexture::draw(GLfloat x, GLfloat y)
{
  // INITIALIZATION

  // Getting shader program
  GLint shader_program;
  glGetIntegerv(GL_CURRENT_PROGRAM, &shader_program);

  // Generate vertex array
  GLuint vertex_array;
  glGenVertexArrays(1, &vertex_array);

  GLint texturecoord_location = glGetAttribLocation(shader_program, "texCoord");
  GLint position_location = glGetAttribLocation(shader_program, "pos");

  // Generate geometry buffers for drawing textured quad
  GLfloat position[8] = { x, y, x + width, y, x + width, y + height, x, y + height };
  GLfloat texcoord[8] = {0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0};

  GLuint vertex_buffer[2];
  glGenBuffers(2, vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer[0]);
  glBufferData(GL_ARRAY_BUFFER, 32, position, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer[1]);
  glBufferData(GL_ARRAY_BUFFER, 32, texcoord, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  // DRAWING
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, textureId);

  glBindVertexArray(vertex_array);
  glEnableVertexAttribArray(texturecoord_location);
  glEnableVertexAttribArray(position_location);

  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer[0]);
  glVertexAttribPointer(position_location, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer[1]);
  glVertexAttribPointer(texturecoord_location, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);

  // DELETING
  glDeleteBuffers(2, vertex_buffer);
  glDeleteVertexArrays(1, &vertex_array);
}

void ViewportEngine::sync(BL::Depsgraph &b_depsgraph, BL::Context &b_context, pxr::HdRenderSettingsMap &renderSettings)
{
  if (!sceneDelegate) {
    sceneDelegate = std::make_unique<BlenderSceneDelegate>(renderIndex.get(), 
      SdfPath::AbsoluteRootPath().AppendElementString("scene"));
  }
  sceneDelegate->Populate(b_depsgraph, b_context);

  for (auto const& setting : renderSettings) {
    renderDelegate->SetRenderSetting(setting.first, setting.second);
  }
}

void ViewportEngine::viewDraw(BL::Depsgraph &b_depsgraph, BL::Context &b_context)
{
  ViewSettings viewSettings(b_context);
  if (viewSettings.get_width() * viewSettings.get_height() == 0) {
    return;
  };

  BL::Scene b_scene = b_depsgraph.scene_eval();
  GfCamera gfCamera = viewSettings.export_camera();

  freeCameraDelegate->SetCamera(gfCamera);
  renderTaskDelegate->SetCameraAndViewport(freeCameraDelegate->GetCameraId(), 
    GfVec4d(viewSettings.border[0][0], viewSettings.border[0][1], viewSettings.border[1][0], viewSettings.border[1][1]));

  if (!b_engine.bl_use_gpu_context()) {
    renderTaskDelegate->SetRendererAov(HdAovTokens->color);
  }
  
  HdTaskSharedPtrVector tasks = renderTaskDelegate->GetTasks();

  if (getRendererPercentDone() == 0.0f) {
    timeBegin = chrono::steady_clock::now();
  }

  b_engine.bind_display_space_shader(b_scene);

  {
    // Release the GIL before calling into hydra, in case any hydra plugins call into python.
    TF_PY_ALLOW_THREADS_IN_SCOPE();
    engine->Execute(renderIndex.get(), &tasks);

    if (!b_engine.bl_use_gpu_context()) {
      texture.setBuffer(renderTaskDelegate->GetRendererAov(HdAovTokens->color));
      texture.draw((GLfloat)viewSettings.border[0][0], (GLfloat)viewSettings.border[0][1]);
    }
  }
  
  b_engine.unbind_display_space_shader();

  chrono::time_point<chrono::steady_clock> timeCurrent = chrono::steady_clock::now();
  chrono::milliseconds elapsedTime = chrono::duration_cast<chrono::milliseconds>(timeCurrent - timeBegin);

  string formattedTime = formatDuration(elapsedTime);

  if (!renderTaskDelegate->IsConverged()) {
    notifyStatus("Time: " + formattedTime + " | Done: " + to_string(int(getRendererPercentDone())) + "%",
                 "Render");
    b_engine.tag_redraw();
  }
  else {
    notifyStatus(("Time: " + formattedTime).c_str(), "Rendering Done");
  }
}

void ViewportEngine::notifyStatus(const string &info, const string &status)
{
  b_engine.update_stats(status.c_str(), info.c_str());
}

}   // namespace blender::render::hydra
