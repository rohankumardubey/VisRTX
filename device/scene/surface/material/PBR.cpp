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

#include "PBR.h"

namespace visrtx {

void PBR::commit()
{
  m_color = getParam<vec3>("color", vec3(1.f));
  m_colorSampler = getParamObject<Sampler>("color");
  m_colorAttribute = getParam<std::string>("color", "");

  m_opacity = getParam<float>("opacity", 1.f);
  m_opacitySampler = getParamObject<Sampler>("opacity");
  m_opacityAttribute = getParam<std::string>("opacity", "");

  m_metalness = getParam<float>("metalness", 0.f);
  m_metalnessSampler = getParamObject<Sampler>("metalness");
  m_metalnessAttribute = getParam<std::string>("metalness", "");

  m_emissive = getParam<vec3>("emissive", vec3(0.f));
  m_emissiveSampler = getParamObject<Sampler>("emissive");
  m_emissiveAttribute = getParam<std::string>("emissive", "");

  m_transmissiveness = getParam<float>("transmissiveness", 0.f);
  m_transmissivenessSampler = getParamObject<Sampler>("transmissiveness");
  m_transmissivenessAttribute = getParam<std::string>("transmissiveness", "");

  m_roughness = getParam<float>("roughness", 0.f);
  m_roughnessSampler = getParamObject<Sampler>("roughness");
  m_roughnessAttribute = getParam<std::string>("roughness", "");
}

MaterialGPUData PBR::gpuData() const
{
  MaterialGPUData retval;

  populateMaterialParameter(
      retval.baseColor, m_color, m_colorSampler, m_colorAttribute);

  if (m_colorSampler && m_colorSampler->numChannels() > 3)
    retval.opacity = 1.f;
  else {
    populateMaterialParameter(
        retval.opacity, m_opacity, m_opacitySampler, m_opacityAttribute);
  }

  populateMaterialParameter(
      retval.metalness, m_metalness, m_metalnessSampler, m_metalnessAttribute);
  populateMaterialParameter(
      retval.emissive, m_emissive, m_emissiveSampler, m_emissiveAttribute);
  populateMaterialParameter(retval.transmissiveness,
      m_transmissiveness,
      m_transmissivenessSampler,
      m_transmissivenessAttribute);
  populateMaterialParameter(
      retval.roughness, m_roughness, m_roughnessSampler, m_roughnessAttribute);

  return retval;
}

} // namespace visrtx
