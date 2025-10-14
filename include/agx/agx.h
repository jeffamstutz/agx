// Copyright 2025 The Khronos Group
// SPDX-License-Identifier: Apache-2.0

#pragma once

// C-style API in C++ for animated geometry export, ANARI-style.

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
