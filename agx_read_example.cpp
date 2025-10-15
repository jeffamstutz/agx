// Copyright 2025 Jefferson Amstutz
// SPDX-License-Identifier: Apache-2.0

// agx
#include "agx/agx_read.h"
// std
#include <cstdio>
#include <vector>

static void printParam(const char *prefix, const AGXParamView &p)
{
  std::printf("%s name='%.*s' isArray=%u ",
      prefix,
      (int)p.nameLength,
      p.name,
      (unsigned)p.isArray);
  if (!p.isArray) {
    std::printf("type=%s bytes=%llu\n",
        anari::toString(p.type),
        (unsigned long long)p.dataBytes);
  } else {
    std::printf("elemType=%s elemCount=%llu bytes=%llu\n",
        anari::toString(p.elementType),
        (unsigned long long)p.elementCount,
        (unsigned long long)p.dataBytes);
  }
}

int main(int argc, char **argv)
{
  if (argc != 2) {
    std::fprintf(stderr, "Usage: %s <file.agxb>\n", argv[0]);
    return 1;
  }

  AGXReader r = agxNewReader(argv[1]);
  if (!r) {
    std::fprintf(stderr, "Failed to open '%s'\n", argv[1]);
    return 2;
  }

  AGXHeader hdr{};
  if (agxReaderGetHeader(r, &hdr) != 0) {
    std::fprintf(stderr, "Failed to read header\n");
    agxReleaseReader(r);
    return 3;
  }

  std::printf("AGXB v%u: timeSteps=%u constants=%u (swap=%u)\n",
      hdr.version,
      hdr.timeSteps,
      hdr.constantParamCount,
      hdr.needByteSwap);

  // Type
  std::printf("   Type: '%s'\n", anari::toString(hdr.objectType));

  // Subtype
  const char *subtype = agxReaderGetSubtype(r);
  std::printf("Subtype: '%s'\n", subtype);

  // Constants
  agxReaderResetConstants(r);
  AGXParamView pv{};
  while (true) {
    int rc = agxReaderNextConstant(r, &pv);
    if (rc < 0) {
      std::fprintf(stderr, "Error reading constants\n");
      break;
    }
    if (rc == 0)
      break;
    printParam("CONST:", pv);
    // If you need to keep the data beyond the next call, copy it now.
  }

  // Time steps
  agxReaderResetTimeSteps(r);
  uint32_t stepIndex = 0, paramCount = 0;
  while (agxReaderBeginNextTimeStep(r, &stepIndex, &paramCount) == 1) {
    std::printf("Time step %u: %u params\n", stepIndex, paramCount);
    for (;;) {
      int rc = agxReaderNextTimeStepParam(r, &pv);
      if (rc < 0) {
        std::fprintf(stderr, "Error reading step params\n");
        break;
      }
      if (rc == 0)
        break;
      printParam("STEP :", pv);
    }
  }

  agxReleaseReader(r);
  return 0;
}
