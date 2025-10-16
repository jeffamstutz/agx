# AGX — Animated Geometry eXport

AGX is a tiny, stb-style API (implemented in C++) for dumping animated geometry
and associated parameters to a file. It mirrors the feel of the ANARI API:
parameters can be set either as constants (for the entire animation) or per time
step, and arrays are typed using ANARI’s logical type enums (ANARIDataType).
This exists to make it easy to create small, portable fixtures or debug dumps of
animated geometry without requiring a renderer.

## Build Requirements

- C++17 compiler
- [ANARI-SDK](https://github.com/KhronosGroup/ANARI-SDK)

## Example Usage

```cpp
#define AGX_WRITE_IMPL
#include "agx/agx_write.h"

#include <vector>
#include <cmath>

int main()
{
  AGXExporter ex = agxNewExporter();

  // Constants across the whole animation
  const float bboxMin[3] = {0.f, 0.f, 0.f};
  const float bboxMax[3] = {1.f, 1.f, 1.f};
  agxSetParameter(ex, "bbox.min", ANARI_FLOAT32_VEC3, bboxMin);
  agxSetParameter(ex, "bbox.max", ANARI_FLOAT32_VEC3, bboxMax);

  // Triangle index buffer (constant)
  std::vector<uint32_t> indices = {0,1,2, 2,3,0};
  agxSetParameterArray1D(ex, "primitive.index", ANARI_UINT32, indices.data(), indices.size());

  // Set number of time steps
  const uint32_t T = 4;
  agxSetTimeStepCount(ex, T);

  // Per-time-step vertex positions and a 'time' scalar
  for (uint32_t t = 0; t < T; ++t) {
    agxBeginTimeStep(ex, t); // no-op; for ANARI-style flow

    std::vector<float> positions(4 * 3); // 4 vertices, each vec3
    float phase = 0.5f * t;
    positions[0] = 0.f; positions[1] = 0.f; positions[2] = std::sin(phase);
    positions[3] = 1.f; positions[4] = 0.f; positions[5] = std::cos(phase);
    positions[6] = 1.f; positions[7] = 1.f; positions[8] = std::sin(phase + 0.3f);
    positions[9] = 0.f; positions[10]= 1.f; positions[11]= std::cos(phase + 0.3f);

    agxSetTimeStepParameterArray1D(ex, t, "vertex.position", ANARI_FLOAT32_VEC3, positions.data(), 4);

    float timeValue = float(t) / float(T - 1);
    agxSetTimeStepParameter(ex, t, "time", ANARI_FLOAT32, &timeValue);

    agxEndTimeStep(ex, t); // no-op
  }

  // Write JSON dump
  int rc = agxWrite(ex, "animated_geometry_dump.agx");

  agxReleaseExporter(ex);
  return rc;
}
```