/*
 * Copyright (c) 2019-2022 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "gpu/gpu_objects.h"
#include "utility/DeferredCommitBuffer.h"
#include "utility/DeferredUploadBuffer.h"
#include "utility/DeviceObjectArray.h"
// optix
#include <optix.h>
#include <optix_stubs.h>
// std
#include <functional>
#include <sstream>
#include <stdexcept>
#include <vector>

constexpr int PAYLOAD_VALUES = 5;
constexpr int ATTRIBUTE_VALUES = 4;

#define OPTIX_CHECK(call)                                                      \
  {                                                                            \
    OptixResult res = call;                                                    \
    if (res != OPTIX_SUCCESS) {                                                \
      std::stringstream ss;                                                    \
      const char *res_str = optixGetErrorName(res);                            \
      ss << "Optix call (" << #call << ") failed with code " << res_str        \
         << " (line " << __LINE__ << ")\n";                                    \
      reportMessage(ANARI_SEVERITY_FATAL_ERROR, "%s", ss.str().c_str());       \
    }                                                                          \
  }

#define OPTIX_CHECK_OBJECT(call, obj)                                          \
  {                                                                            \
    OptixResult res = call;                                                    \
    if (res != OPTIX_SUCCESS) {                                                \
      std::stringstream ss;                                                    \
      const char *res_str = optixGetErrorName(res);                            \
      ss << "Optix call (" << #call << ") failed with code " << res_str        \
         << " (line " << __LINE__ << ")\n";                                    \
      obj->reportMessage(ANARI_SEVERITY_FATAL_ERROR, "%s", ss.str().c_str());  \
    }                                                                          \
  }

#define CUDA_SYNC_CHECK()                                                      \
  {                                                                            \
    cudaDeviceSynchronize();                                                   \
    cudaError_t error = cudaGetLastError();                                    \
    if (error != cudaSuccess) {                                                \
      reportMessage(ANARI_SEVERITY_FATAL_ERROR,                                \
          "error (%s: line %d): %s\n",                                         \
          __FILE__,                                                            \
          __LINE__,                                                            \
          cudaGetErrorString(error));                                          \
    }                                                                          \
  }

#define CUDA_SYNC_CHECK_OBJECT(obj)                                            \
  {                                                                            \
    cudaDeviceSynchronize();                                                   \
    cudaError_t error = cudaGetLastError();                                    \
    if (error != cudaSuccess) {                                                \
      obj->reportMessage(ANARI_SEVERITY_FATAL_ERROR,                           \
          "error (%s: line %d): %s\n",                                         \
          __FILE__,                                                            \
          __LINE__,                                                            \
          cudaGetErrorString(error));                                          \
    }                                                                          \
  }

namespace visrtx {

using ptx_ptr = unsigned char *;

struct DeviceGlobalState
{
  CUcontext cudaContext;
  CUstream stream;
  cudaDeviceProp deviceProps;

  OptixDeviceContext optixContext;

  std::function<void(int, const std::string &, const void *)> messageFunction;

  struct RendererModules
  {
    OptixModule debug{nullptr};
    OptixModule raycast{nullptr};
    OptixModule ambientOcclusion{nullptr};
    OptixModule diffusePathTracer{nullptr};
    OptixModule scivis{nullptr};
  } rendererModules;

  struct IntersectionModules
  {
    OptixModule customIntersectors{nullptr};
  } intersectionModules;

  struct ObjectUpdates
  {
    TimeStamp lastCommitFlush{0};
    TimeStamp lastUploadFlush{0};
    TimeStamp lastBLASChange{0};
    TimeStamp lastTLASChange{0};
  } objectUpdates;

  DeferredCommitBuffer commitBuffer;
  DeferredUploadBuffer uploadBuffer;

  struct DeviceObjectRegistry
  {
    DeviceObjectArray<SamplerGPUData> samplers;
    DeviceObjectArray<GeometryGPUData> geometries;
    DeviceObjectArray<MaterialGPUData> materials;
    DeviceObjectArray<SurfaceGPUData> surfaces;
    DeviceObjectArray<LightGPUData> lights;
    DeviceObjectArray<SpatialFieldGPUData> fields;
    DeviceObjectArray<VolumeGPUData> volumes;
  } registry;

  // Helper methods //

  void flushCommitBuffer();
  void flushUploadBuffer();
};

struct Object;

void buildOptixBVH(std::vector<OptixBuildInput> buildInput,
    DeviceBuffer &bvh,
    OptixTraversableHandle &traversable,
    box3 &bounds,
    Object *obj);

} // namespace visrtx
