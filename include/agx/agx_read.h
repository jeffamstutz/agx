// Copyright 2025 Jefferson Amstutz
// SPDX-License-Identifier: Apache-2.0

#pragma once

// agx_reader.h - Reader API for AGXB (AGX binary) files.

#include <stddef.h>
#include <stdint.h>
// Use ANARI's logical type enums.
#include <anari/frontend/anari_enums.h>
#include <anari/frontend/type_utility.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque reader handle
typedef struct AGXReader_t *AGXReader;

// Header information (copied from file header)
typedef struct AGXHeader
{
  uint32_t version;
  ANARIDataType objectType;
  uint32_t timeSteps;
  uint32_t constantParamCount;

  // Endianness info
  uint32_t endianMarker; // value from file
  uint8_t hostLittleEndian; // 1 if host is little-endian
  uint8_t fileLittleEndian; // 1 if file is little-endian
  uint8_t needByteSwap; // 1 if we need to swap 32/64-bit scalar header fields
  uint8_t reserved;
} AGXHeader;

// A view into a parameter record. The 'data' and 'name' pointers are valid
// until the next Next* call on the same reader (or the reader is destroyed).
typedef struct AGXParamView
{
  const char *name; // not null-terminated guaranteed; see nameLength
  uint32_t nameLength; // length of the name string
  uint8_t isArray; // 0 = single value, 1 = array

  // For single value
  ANARIDataType type; // valid when isArray == 0

  // For arrays
  ANARIDataType elementType; // valid when isArray == 1
  uint64_t elementCount; // valid when isArray == 1

  // Raw bytes for the value or array contents
  const void *data; // pointer to internal buffer
  uint64_t dataBytes; // number of bytes pointed to by 'data'
} AGXParamView;

// Open/close
AGXReader agxNewReader(const char *filename);
void agxReleaseReader(AGXReader r);

// Header
// Returns 0 on success; nonzero on error. 'out' is filled on success.
int agxReaderGetHeader(AGXReader r, AGXHeader *out);

// Get object subtype (as set by writer, or "" if none was set).
const char *agxReaderGetSubtype(AGXReader r);

// Constant parameters iteration
// Resets iteration to the first constant parameter.
void agxReaderResetConstants(AGXReader r);

// Returns 1 and fills 'out' on success (a parameter was read).
// Returns 0 when no more constants are available.
// Returns -1 on error.
int agxReaderNextConstant(AGXReader r, AGXParamView *out);

// Time step iteration
// Positions reader to the next time step. Returns 1 on success,
// 0 if there are no more time steps, -1 on error.
// On success, fills outIndex and outParamCount.
int agxReaderBeginNextTimeStep(
    AGXReader r, uint32_t *outIndex, uint32_t *outParamCount);

// Read next parameter within the current time step.
// Returns 1 and fills 'out' when a parameter is produced.
// Returns 0 when the current time step has no more parameters.
// Returns -1 on error.
int agxReaderNextTimeStepParam(AGXReader r, AGXParamView *out);

// Optional: skip any remaining parameters in the current time step.
void agxReaderSkipRemainingTimeStep(AGXReader r);

// Reset time step iteration to the first time step.
void agxReaderResetTimeSteps(AGXReader r);

#ifdef __cplusplus
} // extern "C"
#endif
