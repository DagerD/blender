/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2011-2022 Blender Foundation */

#pragma once

#include <chrono>

#include "engine.h"

namespace usdhydra {

class FinalEngine : public Engine {
public:
  using Engine::Engine;
  void sync(BL::Depsgraph &b_depsgraph, BL::Context &b_context, pxr::HdRenderSettingsMap &renderSettings) override;
  void render(BL::Depsgraph &b_depsgraph);

private:
  void getResolution(BL::RenderSettings b_render, int &width, int &height);
  void updateRenderResult(std::map<std::string, std::vector<float>> &render_images, const std::string &layerName, int width, int height);
  void notifyStatus(float progress, const std::string &title, const std::string &info);
};

}   // namespace usdhydra
