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

#include "SciVis.h"
#include "ParameterInfo.h"
// ptx
#include "SciVis_ptx.h"

namespace visrtx {

static const std::vector<HitgroupFunctionNames> g_scivisHitNames = {
    {"__closesthit__primary", ""},
    {"__closesthit__shadow", "__anyhit__shadow"}};

static const std::vector<std::string> g_scivisMissNames = {
    "__miss__", "__miss__"};

void SciVis::commit()
{
  Renderer::commit();
  m_lightFalloff = std::clamp(getParam<float>("lightFalloff", 0.25f), 0.f, 1.f);
  m_aoSamples = std::clamp(getParam<int>("ambientSamples", 0), 0, 256);
  m_aoColor = glm::clamp(
      getParam<vec3>("ambientColor", vec3(1.f)), vec3(0.f), vec3(1.f));
  m_aoIntensity = getParam<float>("ambientIntensity", 1.f);
}

void SciVis::populateFrameData(FrameGPUData &fd) const
{
  Renderer::populateFrameData(fd);
  auto &scivis = fd.renderer.params.scivis;
  scivis.lightFalloff = m_lightFalloff;
  scivis.aoSamples = m_aoSamples;
  scivis.aoColor = m_aoColor;
  scivis.aoIntensity = m_aoIntensity;
}

OptixModule SciVis::optixModule() const
{
  return deviceState()->rendererModules.scivis;
}

anari::Span<const HitgroupFunctionNames> SciVis::hitgroupSbtNames() const
{
  return anari::make_Span(g_scivisHitNames.data(), g_scivisHitNames.size());
}

anari::Span<const std::string> SciVis::missSbtNames() const
{
  return anari::make_Span(g_scivisMissNames.data(), g_scivisMissNames.size());
}

ptx_ptr SciVis::ptx()
{
  return SciVis_ptx;
}

const void *SciVis::getParameterInfo(std::string_view paramName,
    ANARIDataType paramType,
    std::string_view infoName,
    ANARIDataType infoType)
{
  if (paramName == "lightFalloff" && paramType == ANARI_FLOAT32) {
    static const ParameterInfo param(
        false, "energy falloff when evaluating lights", 0.25f, 0.f, 1.f);
    return param.fromString(infoName, infoType);
  } else if (paramName == "ambientSamples" && paramType == ANARI_INT32) {
    static const ParameterInfo param(
        false, "number of ambient occlusion samples each frame", 0, 0, 256);
    return param.fromString(infoName, infoType);
  } else if (paramName == "ambientIntensity" && paramType == ANARI_FLOAT32) {
    static const ParameterInfo param(false, "ambient lighting intensity", 1.f);
    return param.fromString(infoName, infoType);
  } else if (paramName == "ambientColor" && paramType == ANARI_FLOAT32_VEC3) {
    static const ParameterInfo param(
        false, "ambient lighting color", vec3(1.f), vec3(0.f), vec3(1.f));
    return param.fromString(infoName, infoType);
  }

  return nullptr;
}

} // namespace visrtx
