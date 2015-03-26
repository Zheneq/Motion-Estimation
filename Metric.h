#ifndef MOTIONESTIMATION_METRIC_H_
#define MOTIONESTIMATION_METRIC_H_

#include "types.h"

/// Compute SAD metric between two blocks of 16x16 pixels
long
GetErrorSAD_16x16(const BYTE *block1,
                  const BYTE *block2,
                  const int stride);

/// Compute SAD metric between two blocks of 8x8 pixels
long
GetErrorSAD_8x8(const BYTE *block1,
                const BYTE *block2,
                const int stride);

#endif
