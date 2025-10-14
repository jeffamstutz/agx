// Copyright 2025 The Khronos Group
// SPDX-License-Identifier: Apache-2.0

#include "agx/agx.h"

// std
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
// anari
#include <anari/frontend/type_utility.h>

// Internal representation of parameter data
struct ParamData
{
  bool isArray{false};
  ANARIDataType type{(ANARIDataType)0}; // for single value
  ANARIDataType elementType{(ANARIDataType)0}; // for arrays
  uint64_t elementCount{0}; // for arrays
  std::vector<uint8_t> bytes; // raw bytes
};

struct AGXExporter_t
{
  uint32_t timeSteps{0};
  std::unordered_map<std::string, ParamData> constants;
  std::vector<std::unordered_map<std::string, ParamData>>
      perTimeStep; // size = timeSteps
};

static inline uint32_t clampToValidIndex(uint32_t idx, uint32_t max)
{
  return std::min(idx, max);
}

// Basic sizeof mapping for common ANARIDataType values
size_t agxSizeOf(ANARIDataType t)
{
  return anari::sizeOf(t);
}

const char *agxDataTypeToString(ANARIDataType t)
{
  return anari::toString(t);
}

static void copyBytes(ParamData &dst, const void *src, size_t nbytes)
{
  dst.bytes.resize(nbytes);
  if (src && nbytes > 0)
    std::memcpy(dst.bytes.data(), src, nbytes);
}

// JSON helpers
static std::string jsonEscape(const std::string &s)
{
  std::ostringstream os;
  for (char c : s) {
    switch (c) {
    case '\"':
      os << "\\\"";
      break;
    case '\\':
      os << "\\\\";
      break;
    case '\b':
      os << "\\b";
      break;
    case '\f':
      os << "\\f";
      break;
    case '\n':
      os << "\\n";
      break;
    case '\r':
      os << "\\r";
      break;
    case '\t':
      os << "\\t";
      break;
    default:
      if (static_cast<unsigned char>(c) < 0x20) {
        os << "\\u" << std::hex << std::setw(4) << std::setfill('0')
           << (int)(unsigned char)c;
      } else {
        os << c;
      }
      break;
    }
  }
  return os.str();
}

template <typename T>
static void dumpAsNumbers(
    std::ostream &os, const uint8_t *bytes, size_t countScalars)
{
  const T *v = reinterpret_cast<const T *>(bytes);
  for (size_t i = 0; i < countScalars; ++i) {
    if (i)
      os << ", ";
    if constexpr (std::is_floating_point<T>::value) {
      os << std::setprecision(7) << v[i];
    } else if constexpr (std::is_same<T, uint8_t>::value) {
      os << (unsigned)v[i];
    } else {
      os << v[i];
    }
  }
}

static void dumpRawBytes(std::ostream &os, const uint8_t *bytes, size_t nbytes)
{
  for (size_t i = 0; i < nbytes; ++i) {
    if (i)
      os << ", ";
    os << (unsigned)bytes[i];
  }
}

// Returns (scalarCount, scalarKind) where scalarKind is a function pointer that
// emits numbers
enum class ScalarKind
{
  U8,
  I32,
  U32,
  I64,
  U64,
  F32,
  F64,
  UNKNOWN
};

static std::pair<size_t, ScalarKind> scalarInfo(ANARIDataType t)
{
  switch (t) {
  case ANARI_BOOL:
    return {1, ScalarKind::U8};
  case ANARI_INT8:
    return {1, ScalarKind::U8}; // dump as unsigned for readability
  case ANARI_UINT8:
    return {1, ScalarKind::U8};
  case ANARI_INT16:
    return {1, ScalarKind::I32}; // promote to 32-bit for dump
  case ANARI_UINT16:
    return {1, ScalarKind::U32};
  case ANARI_INT32:
    return {1, ScalarKind::I32};
  case ANARI_UINT32:
    return {1, ScalarKind::U32};
  case ANARI_INT64:
    return {1, ScalarKind::I64};
  case ANARI_UINT64:
    return {1, ScalarKind::U64};
  case ANARI_FLOAT32:
    return {1, ScalarKind::F32};
  case ANARI_FLOAT64:
    return {1, ScalarKind::F64};

  case ANARI_FLOAT32_VEC2:
    return {2, ScalarKind::F32};
  case ANARI_FLOAT32_VEC3:
    return {3, ScalarKind::F32};
  case ANARI_FLOAT32_VEC4:
    return {4, ScalarKind::F32};

  case ANARI_INT32_VEC2:
    return {2, ScalarKind::I32};
  case ANARI_INT32_VEC3:
    return {3, ScalarKind::I32};
  case ANARI_INT32_VEC4:
    return {4, ScalarKind::I32};

  case ANARI_UINT32_VEC2:
    return {2, ScalarKind::U32};
  case ANARI_UINT32_VEC3:
    return {3, ScalarKind::U32};
  case ANARI_UINT32_VEC4:
    return {4, ScalarKind::U32};

  case ANARI_FLOAT32_MAT3:
    return {9, ScalarKind::F32};
  case ANARI_FLOAT32_MAT4:
    return {16, ScalarKind::F32};

  default:
    return {0, ScalarKind::UNKNOWN};
  }
}

static void dumpTypedScalars(
    std::ostream &os, ANARIDataType t, const uint8_t *bytes, size_t nbytes)
{
  const auto [count, kind] = scalarInfo(t);
  if (count == 0 || nbytes == 0) {
    // Unknown type or empty; fallback to bytes
    dumpRawBytes(os, bytes, nbytes);
    return;
  }

  switch (kind) {
  case ScalarKind::U8:
    dumpAsNumbers<uint8_t>(os, bytes, count);
    break;
  case ScalarKind::I32:
    dumpAsNumbers<int32_t>(os, bytes, count);
    break;
  case ScalarKind::U32:
    dumpAsNumbers<uint32_t>(os, bytes, count);
    break;
  case ScalarKind::I64:
    dumpAsNumbers<int64_t>(os, bytes, count);
    break;
  case ScalarKind::U64:
    dumpAsNumbers<uint64_t>(os, bytes, count);
    break;
  case ScalarKind::F32:
    dumpAsNumbers<float>(os, bytes, count);
    break;
  case ScalarKind::F64:
    dumpAsNumbers<double>(os, bytes, count);
    break;
  default:
    dumpRawBytes(os, bytes, nbytes);
    break;
  }
}

static void writeParamJSON(std::ostream &os, const ParamData &p, int indent)
{
  auto ind = std::string(indent, ' ');
  os << ind << "{\n";
  if (!p.isArray) {
    os << ind << "  \"type\": \"" << agxDataTypeToString(p.type) << "\",\n";
    os << ind << "  \"value\": ";
    os << "[";
    dumpTypedScalars(os, p.type, p.bytes.data(), p.bytes.size());
    os << "]";
  } else {
    size_t elemBytes = agxSizeOf(p.elementType);
    os << ind << "  \"arrayElementType\": \""
       << agxDataTypeToString(p.elementType) << "\",\n";
    os << ind << "  \"elementCount\": " << p.elementCount << ",\n";
    os << ind << "  \"data\": [";
    if (elemBytes > 0 && p.elementCount > 0) {
      // Flatten elements as scalar sequences
      const uint8_t *ptr = p.bytes.data();
      for (uint64_t e = 0; e < p.elementCount; ++e) {
        if (e)
          os << ", ";
        // Either print element as [ ... ] or flattened; we print as nested
        // arrays for readability.
        os << "[";
        dumpTypedScalars(os, p.elementType, ptr + e * elemBytes, elemBytes);
        os << "]";
      }
    }
    os << "]";
  }
  os << "\n" << ind << "}";
}

// Public API implementations

extern "C" {

AGXExporter agxNewExporter()
{
  return new (std::nothrow) AGXExporter_t{};
}

void agxReleaseExporter(AGXExporter exporter)
{
  delete exporter;
}

void agxSetTimeStepCount(AGXExporter exporter, uint32_t count)
{
  if (!exporter)
    return;
  exporter->timeSteps = count;
  exporter->perTimeStep.resize(count);
}

uint32_t agxGetTimeStepCount(AGXExporter exporter)
{
  if (!exporter)
    return 0;
  return exporter->timeSteps;
}

void agxBeginTimeStep(AGXExporter exporter, uint32_t /*timeStepIndex*/)
{
  // No-op, provided for ANARI-style flow.
  (void)exporter;
}

void agxEndTimeStep(AGXExporter exporter, uint32_t /*timeStepIndex*/)
{
  // No-op, provided for ANARI-style flow.
  (void)exporter;
}

void agxSetParameter(AGXExporter exporter,
    const char *name,
    ANARIDataType type,
    const void *value)
{
  if (!exporter || !name)
    return;
  ParamData p;
  p.isArray = false;
  p.type = type;
  const size_t nbytes = agxSizeOf(type);
  copyBytes(p, value, nbytes);
  exporter->constants[std::string(name)] = std::move(p);
}

void agxSetParameterArray1D(AGXExporter exporter,
    const char *name,
    ANARIDataType elementType,
    const void *data,
    uint64_t elementCount)
{
  if (!exporter || !name)
    return;
  ParamData p;
  p.isArray = true;
  p.elementType = elementType;
  p.elementCount = elementCount;
  const size_t elemBytes = agxSizeOf(elementType);
  const size_t total = elemBytes * elementCount;
  copyBytes(p, data, total);
  exporter->constants[std::string(name)] = std::move(p);
}

void agxSetTimeStepParameter(AGXExporter exporter,
    uint32_t timeStepIndex,
    const char *name,
    ANARIDataType type,
    const void *value)
{
  if (!exporter || !name)
    return;
  timeStepIndex = clampToValidIndex(
      timeStepIndex, exporter->timeSteps ? exporter->timeSteps - 1 : 0);
  if (exporter->perTimeStep.size() != exporter->timeSteps)
    exporter->perTimeStep.resize(exporter->timeSteps);

  ParamData p;
  p.isArray = false;
  p.type = type;
  const size_t nbytes = agxSizeOf(type);
  copyBytes(p, value, nbytes);
  exporter->perTimeStep[timeStepIndex][std::string(name)] = std::move(p);
}

void agxSetTimeStepParameterArray1D(AGXExporter exporter,
    uint32_t timeStepIndex,
    const char *name,
    ANARIDataType elementType,
    const void *data,
    uint64_t elementCount)
{
  if (!exporter || !name)
    return;
  timeStepIndex = clampToValidIndex(
      timeStepIndex, exporter->timeSteps ? exporter->timeSteps - 1 : 0);
  if (exporter->perTimeStep.size() != exporter->timeSteps)
    exporter->perTimeStep.resize(exporter->timeSteps);

  ParamData p;
  p.isArray = true;
  p.elementType = elementType;
  p.elementCount = elementCount;
  const size_t elemBytes = agxSizeOf(elementType);
  const size_t total = elemBytes * elementCount;
  copyBytes(p, data, total);
  exporter->perTimeStep[timeStepIndex][std::string(name)] = std::move(p);
}

int agxWrite(AGXExporter exporter, const char *filename)
{
  if (!exporter || !filename)
    return 1;
  std::ofstream ofs(filename, std::ios::out | std::ios::trunc);
  if (!ofs.is_open())
    return 2;

  ofs << "{\n";
  ofs << "  \"timeSteps\": " << exporter->timeSteps << ",\n";

  ofs << "  \"constants\": {\n";
  bool first = true;
  for (const auto &kv : exporter->constants) {
    if (!first)
      ofs << ",\n";
    first = false;
    ofs << "    \"" << jsonEscape(kv.first) << "\": ";
    writeParamJSON(ofs, kv.second, 6);
  }
  ofs << "\n  },\n";

  ofs << "  \"timeStepData\": [\n";
  for (uint32_t i = 0; i < exporter->timeSteps; ++i) {
    ofs << "    {\n";
    ofs << "      \"index\": " << i << ",\n";
    ofs << "      \"params\": {\n";
    bool firstP = true;
    const auto &m = exporter->perTimeStep[i];
    for (const auto &kv : m) {
      if (!firstP)
        ofs << ",\n";
      firstP = false;
      ofs << "        \"" << jsonEscape(kv.first) << "\": ";
      writeParamJSON(ofs, kv.second, 10);
    }
    ofs << "\n      }\n";
    ofs << "    }";
    if (i + 1 < exporter->timeSteps)
      ofs << ",";
    ofs << "\n";
  }
  ofs << "  ]\n";
  ofs << "}\n";

  return 0;
}

} // extern "C"