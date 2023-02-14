/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2011-2022 Blender Foundation */

#pragma once

#include <chrono>
#include <string>

#include "BKE_image.h"
#include "BKE_image_save.h"

namespace blender::render::hydra {

std::string formatDuration(std::chrono::milliseconds secs);
std::string cache_image(Main *bmain,
                        Scene *scene,
                        Image *image,
                        ImageUser *iuser,
                        ImageSaveOptions *opts,
                        ReportList *reports);

} // namespace blender::render::hydra
