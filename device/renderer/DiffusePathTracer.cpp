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

#include "DiffusePathTracer.h"
#include "ParameterInfo.h"
// ptx
#include "DiffusePathTracer_ptx.h"

namespace visrtx {

void DiffusePathTracer::commit()
{
  Renderer::commit();
  m_maxDepth = std::clamp(getParam<int>("maxDepth", 5), 1, 256);
  m_R = getParam<float>("R", 0.5f);
}

void DiffusePathTracer::populateFrameData(FrameGPUData &fd) const
{
  Renderer::populateFrameData(fd);
  fd.renderer.params.dpt.maxDepth = m_maxDepth;
  fd.renderer.params.dpt.R = m_R;
}

OptixModule DiffusePathTracer::optixModule() const
{
  return deviceState()->rendererModules.diffusePathTracer;
}

ptx_ptr DiffusePathTracer::ptx()
{
  return DiffusePathTracer_ptx;
}

const void *DiffusePathTracer::getParameterInfo(std::string_view paramName,
    ANARIDataType paramType,
    std::string_view infoName,
    ANARIDataType infoType)
{
  if (paramName == "maxDepth" && paramType == ANARI_INT32) {
    static const ParameterInfo param(
        false, "maximum per-pixel path depth", 5, 1, 256);
    return param.fromString(infoName, infoType);
  } else if (paramName == "R" && paramType == ANARI_FLOAT32) {
    static const ParameterInfo param(
        false, "per-bounce energy falloff factor", 0.5f, 0.f, 1.f);
    return param.fromString(infoName, infoType);
  }

  return nullptr;
}

} // namespace visrtx
