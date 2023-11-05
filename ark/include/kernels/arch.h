// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef ARK_KERNELS_ARCH_H_
#define ARK_KERNELS_ARCH_H_

#include "device.h"
#include "static_math.h"

namespace ark {

struct Arch {
    static const int ThreadsPerWarp = 32;
};

DEVICE int warp_id() {
    return threadIdx.x >> math::log2_up<Arch::ThreadsPerWarp>::value;
}

}  // namespace ark

#endif  // ARK_KERNELS_ARCH_H_
