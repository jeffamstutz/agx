// Copyright 2025 Jefferson Amstutz
// SPDX-License-Identifier: Apache-2.0

#pragma once

// C-style API in C++ for animated geometry export, ANARI-style.

// File format (v1, host-endian; an endianness marker is included):
//   Header:
//     char[4]   magic = "AGXB"
//     uint32_t  version = 1
//     uint32_t  endianMarker = 0x01020304
//     uint32_t  objectType
//     uint32_t  timeSteps
//     uint32_t  constantParamCount
//
//   Optional subtype string:
//     uint32_t  subtypeLen
//     char[]    subtype (subtypeLen bytes, not null-terminated)
//
//   Constant parameter records (constantParamCount times):
//     uint32_t  nameLen
//     char[]    name (nameLen bytes, not null-terminated)
//     uint8_t   isArray (0 = value, 1 = array)
//     if isArray == 0:
//       uint32_t  type       (ANARIDataType)
//       uint32_t  valueBytes (N)
//       uint8_t[] value (N bytes)
//     else:
//       uint32_t  elementType (ANARIDataType)
//       uint64_t  elementCount
//       uint64_t  dataBytes (M)
//       uint8_t[] data (M bytes; M == elementCount * sizeof(elementType))
//
//   For each time step (timeSteps times):
//     uint32_t  timeStepIndex
//     uint32_t  paramCount
//     paramCount parameter records (same layout as above)
//
// Notes:
// - Values are written in host endianness; the endianMarker lets a reader
// detect endianness.
// - Unknown ANARIDataType values will have size 0 and thus write zero bytes of
// payload.
// - Strings are stored without a trailing NUL.

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Use ANARI's logical type enums.
#include <anari/frontend/anari_enums.h>

// Opaque exporter handle
typedef struct AGXExporter_t *AGXExporter;

// Create/destroy exporter
AGXExporter agxNewExporter();
void agxReleaseExporter(AGXExporter exporter);

// Set object subtype (optional, default = "")
void agxSetObjectSubtype(AGXExporter exporter, const char *subtype);

// Animation time steps
void agxSetTimeStepCount(AGXExporter exporter, uint32_t count);
uint32_t agxGetTimeStepCount(AGXExporter exporter);

// Optional begin/end bracketing of per-time step edits (no-op but keeps
// ANARI-like style)
void agxBeginTimeStep(AGXExporter exporter, uint32_t timeStepIndex);
void agxEndTimeStep(AGXExporter exporter, uint32_t timeStepIndex);

// Set constant parameters (across the whole animation)
void agxSetParameter(AGXExporter exporter,
    const char *name,
    ANARIDataType type,
    const void *value /* pointer to a single value of 'type' */);

void agxSetParameterArray1D(AGXExporter exporter,
    const char *name,
    ANARIDataType elementType,
    const void *data,
    uint64_t elementCount);

// Set per-time-step parameters
void agxSetTimeStepParameter(AGXExporter exporter,
    uint32_t timeStepIndex,
    const char *name,
    ANARIDataType type,
    const void *value);

void agxSetTimeStepParameterArray1D(AGXExporter exporter,
    uint32_t timeStepIndex,
    const char *name,
    ANARIDataType elementType,
    const void *data,
    uint64_t elementCount);

// Write out dump to a file (returns 0 on success, nonzero on error)
int agxWrite(AGXExporter exporter, const char *filename);

// Helpers
size_t agxSizeOf(ANARIDataType type);
const char *agxDataTypeToString(ANARIDataType type);

#ifdef __cplusplus
} // extern "C"
#endif
