// Copyright 2025 Jefferson Amstutz
// SPDX-License-Identifier: Apache-2.0

// std
#include <cmath>
#include <vector>
// agx
#include "agx/agx_write.h"

int main()
{
  AGXExporter ex = agxNewExporter();

  // Set constants
  const float bboxMin[3] = {0.f, 0.f, 0.f};
  const float bboxMax[3] = {1.f, 1.f, 1.f};
  agxSetParameter(ex, "bbox.min", ANARI_FLOAT32_VEC3, bboxMin);
  agxSetParameter(ex, "bbox.max", ANARI_FLOAT32_VEC3, bboxMax);

  // Constant index array (triangles)
  std::vector<uint32_t> index = {0, 1, 2, 2, 3, 0};
  agxSetParameterArray1D(
      ex, "indices", ANARI_UINT32, index.data(), index.size());

  // Time steps
  const uint32_t T = 4;
  agxSetTimeStepCount(ex, T);

  // Per-time-step positions
  std::vector<float> positions;
  positions.resize(4 * 3);

  for (uint32_t t = 0; t < T; ++t) {
    agxBeginTimeStep(ex, t);

    // 4 vertices, each vec3
    float phase = 0.5f * t;
    positions[0] = 0.f;
    positions[1] = 0.f;
    positions[2] = std::sin(phase);
    positions[3] = 1.f;
    positions[4] = 0.f;
    positions[5] = std::cos(phase);
    positions[6] = 1.f;
    positions[7] = 1.f;
    positions[8] = std::sin(phase + 0.3f);
    positions[9] = 0.f;
    positions[10] = 1.f;
    positions[11] = std::cos(phase + 0.3f);

    agxSetTimeStepParameterArray1D(
        ex, t, "positions", ANARI_FLOAT32_VEC3, positions.data(), 4);

    // Also a per-time-step single value
    float timeValue = float(t) / float(T - 1);
    agxSetTimeStepParameter(ex, t, "time", ANARI_FLOAT32, &timeValue);

    agxEndTimeStep(ex, t);
  }

  // Write file
  int rc = agxWrite(ex, "animated_geometry_dump.agxb");

  agxReleaseExporter(ex);
  return rc;
}