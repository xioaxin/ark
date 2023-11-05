// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef ARK_KERNELS_SYNC_H_
#define ARK_KERNELS_SYNC_H_

#include "arch.h"
#include "device.h"
#include "static_math.h"

namespace ark {

namespace sync {

struct State {
    volatile int flag;
    int cnt;
    int is_add;
    int clks_cnt;
};

}  // namespace sync

// Synchronize multiple thread blocks inside a kernel. Guarantee that all
// previous work of all threads in cooperating blocks is finished and
// visible to all threads in the device.
template <int BlockNum>
DEVICE void sync_gpu(sync::State &state) {
    constexpr int MaxOldCnt = BlockNum - 1;
    __syncthreads();
    if (BlockNum == 1) {
        return;
    }
    if (threadIdx.x == 0) {
        // Make sure that all threads in this block have done `__threadfence()`
        // before to flip `flag`.
        __threadfence();
        int is_add_ = state.is_add ^ 1;
        if (is_add_) {
            if (atomicAdd(&state.cnt, 1) == MaxOldCnt) {
                state.flag = 1;
            }
            while (!state.flag) {
            }
        } else {
            if (atomicSub(&state.cnt, 1) == 1) {
                state.flag = 0;
            }
            while (state.flag) {
            }
        }
        state.is_add = is_add_;
    }
    // We need sync here because only a single thread is checking whether
    // the flag is flipped.
    __syncthreads();
}

// Synchronize a group of warps.
// This function replaces `__syncthreads()` of legacy kernel implementations.
// It is needed because in normal practices to implement a kernel, each thread
// block typically processes a single unit task (e.g. one tile) so it is common
// to synchronize all co-working threads via `__syncthreads()`, however in our
// case, we often run multiple tasks or tiles in a single thread block, so we
// need a function which lets each tile to synchronize their own using threads
// only. Since `__syncthreads()` synchronize the entire threads in a thread
// block, we implement a finer-grained version of this via `barrier.sync` PTX
// instruction.
template <int NumWarps>
DEVICE void sync_warps() {
    static_assert(math::is_pow2<NumWarps>::value == 1, "");
    static_assert(Arch::ThreadsPerWarp == 32, "");
    if constexpr (NumWarps == 1) {
        __syncwarp();
    } else if constexpr (NumWarps == 2) {
        static_assert(
            ARK_THREADS_PER_BLOCK <= 512,
            "2-warp barrier is not supported for block sizes larger than 512");
        asm volatile("barrier.sync %0, 64;" ::"r"((threadIdx.x >> 6) + 8));
    } else if constexpr (NumWarps == 4) {
        asm volatile("barrier.sync %0, 128;" ::"r"((threadIdx.x >> 7) + 8));
    } else if constexpr (NumWarps == 8) {
        asm volatile("barrier.sync %0, 256;" ::"r"((threadIdx.x >> 8) + 8));
    } else if constexpr (NumWarps == 16) {
        asm volatile("barrier.sync %0, 512;" ::"r"((threadIdx.x >> 9) + 8));
    } else if constexpr (NumWarps == 32) {
        // If we sync 1024 threads, it means we sync all threads in a thread
        // block because the maximum number of threads per block is 1024 in
        // all NVIDIA devices. Therefore, we do not check `threadIdx.x` and
        // just use barrier 8.
        asm volatile("barrier.sync 8, 1024;");
    }
}

}  // namespace ark

#endif  // ARK_KERNELS_SYNC_H_
