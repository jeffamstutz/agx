// Copyright 2025 Jefferson Amstutz
// SPDX-License-Identifier: Apache-2.0

// A small tool to print the header information of an .agxb (AGX binary) file.
//
// Usage:  ./agxb_info path/to/file.agxb
//
// Header format (v1):
//   char[4]   magic = "AGXB"
//   uint32_t  version = 1
//   uint32_t  endianMarker = 0x01020304 (written in host endianness)
//   uint32_t  timeSteps
//   uint32_t  constantParamCount
//
// Notes:
// - File values are stored in the writer's host endianness; this tool detects
//   and compensates endianness via the endianMarker.

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

static bool hostIsLittleEndian()
{
  uint16_t x = 1;
  return *reinterpret_cast<uint8_t *>(&x) == 1;
}

static uint32_t bswap32(uint32_t v)
{
  return ((v & 0x000000FFu) << 24) | ((v & 0x0000FF00u) << 8)
      | ((v & 0x00FF0000u) >> 8) | ((v & 0xFF000000u) >> 24);
}

static bool readExact(std::FILE *f, void *dst, size_t n)
{
  return std::fread(dst, 1, n, f) == n;
}

static const char *endianStr(bool little)
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
  std::FILE *f = std::fopen(path, "rb");
  if (!f) {
    std::perror("fopen");
    return 2;
  }

  char magic[4];
  if (!readExact(f, magic, sizeof(magic))) {
    std::fprintf(stderr, "Error: failed to read magic bytes\n");
    std::fclose(f);
    return 3;
  }

  if (std::memcmp(magic, "AGXB", 4) != 0) {
    std::fprintf(stderr, "Error: not an agxb file (bad magic)\n");
    std::fclose(f);
    return 4;
  }

  uint32_t version = 0;
  uint32_t endianMarker = 0;
  uint32_t timeSteps = 0;
  uint32_t constantParamCount = 0;

  if (!readExact(f, &version, sizeof(version))
      || !readExact(f, &endianMarker, sizeof(endianMarker))
      || !readExact(f, &timeSteps, sizeof(timeSteps))
      || !readExact(f, &constantParamCount, sizeof(constantParamCount))) {
    std::fprintf(stderr, "Error: incomplete header\n");
    std::fclose(f);
    return 5;
  }

  // Determine if we need to byte-swap based on the endian marker
  bool needSwap = false;
  if (endianMarker == 0x01020304u) {
    needSwap = false;
  } else if (bswap32(endianMarker) == 0x01020304u) {
    needSwap = true;
  } else {
    std::fprintf(stderr, "Error: bad endian marker (0x%08x)\n", endianMarker);
    std::fclose(f);
    return 6;
  }

  if (needSwap) {
    version = bswap32(version);
    endianMarker = bswap32(endianMarker); // becomes 0x01020304
    timeSteps = bswap32(timeSteps);
    constantParamCount = bswap32(constantParamCount);
  }

  bool hostLittle = hostIsLittleEndian();
  bool fileLittle = needSwap ? !hostLittle : hostLittle;

  std::cout << "AGXB header information\n";
  std::cout << "  magic                 : " << std::string(magic, 4) << "\n";
  std::cout << "  version               : " << version << "\n";
  std::cout << "  endian marker         : 0x" << std::hex << std::uppercase
            << (unsigned)endianMarker << std::dec << std::nouppercase << "\n";
  std::cout << "  host endianness       : " << endianStr(hostLittle) << "\n";
  std::cout << "  file endianness       : " << endianStr(fileLittle) << "\n";
  std::cout << "  byte swap needed      : " << (needSwap ? "yes" : "no")
            << "\n";
  std::cout << "  timeSteps             : " << timeSteps << "\n";
  std::cout << "  constantParamCount    : " << constantParamCount << "\n";

  std::fclose(f);
  return 0;
}
