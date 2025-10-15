// Copyright 2025 Jefferson Amstutz
// SPDX-License-Identifier: Apache-2.0

// Prints header information of an .agxb (AGX binary) file using the AGX reader
// API.

// agx
#include "agx/agx_read.h"
// std
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <iostream>

static const char *endianStr(uint8_t little)
{
  return little ? "little-endian" : "big-endian";
}

int main(int argc, char **argv)
{
  if (argc != 2) {
    std::fprintf(stderr, "Usage: %s <file.agxb>\n", argv[0]);
    return 1;
  }

  const char *path = argv[1];
  AGXReader r = agxNewReader(path);
  if (!r) {
    std::fprintf(stderr, "Error: failed to open or parse '%s'\n", path);
    return 2;
  }

  AGXHeader hdr{};
  if (agxReaderGetHeader(r, &hdr) != 0) {
    std::fprintf(stderr, "Error: failed to read header from '%s'\n", path);
    agxReleaseReader(r);
    return 3;
  }

  std::cout << "AGXB header information (via reader API)\n";
  std::cout << "  version               : " << hdr.version << "\n";
  std::cout << "  endian marker         : 0x" << std::hex << std::uppercase
            << std::setw(8) << std::setfill('0') << (unsigned)hdr.endianMarker
            << std::dec << std::nouppercase << "\n";
  std::cout << "  host endianness       : " << endianStr(hdr.hostLittleEndian)
            << "\n";
  std::cout << "  file endianness       : " << endianStr(hdr.fileLittleEndian)
            << "\n";
  std::cout << "  byte swap needed      : " << (hdr.needByteSwap ? "yes" : "no")
            << "\n";
  std::cout << "  objectType            : " << anari::toString(hdr.objectType)
            << "\n";
  std::cout << "  timeSteps             : " << hdr.timeSteps << "\n";
  std::cout << "  constantParamCount    : " << hdr.constantParamCount << "\n";

  // Subtype
  const char *subtype = agxReaderGetSubtype(r);
  std::cout << "  subtype               : '" << subtype << "'\n";

  agxReleaseReader(r);

  return 0;
}
