// Copyright 2022 Google Inc. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the COPYING file in the root of the source
// tree. An additional intellectual property rights grant can be found
// in the file PATENTS. All contributing project authors may
// be found in the AUTHORS file in the root of the source tree.
// -----------------------------------------------------------------------------
//
// Gamma correction utilities.

#ifndef WEBP_SHARPYUV_SHARPYUV_GAMMA_H_
#define WEBP_SHARPYUV_SHARPYUV_GAMMA_H_

#include "sharpyuv/sharpyuv.h"
#include "src/webp/types.h"

#ifdef __cplusplus
extern "C" {
#endif



void SharpYuvInitGammaTables(void);


uint32_t SharpYuvGammaToLinear(uint16_t v, int bit_depth,
                               SharpYuvTransferFunctionType transfer_type);


uint16_t SharpYuvLinearToGamma(uint32_t value, int bit_depth,
                               SharpYuvTransferFunctionType transfer_type);

#ifdef __cplusplus
}
#endif

#endif
