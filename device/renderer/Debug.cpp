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

#include "Debug.h"
// ptx
#include "Debug_ptx.h"

namespace visrtx {

static const std::vector<HitgroupFunctionNames> g_debugHitNames = {
    {"__closesthit__surface", ""}, {"__closesthit__volume", ""}};

static const std::vector<std::string> g_debugMissNames = {
    "__miss__", "__miss__"};

static Debug::Method methodFromString(const std::string &name)
{
  if (name == "primID")
    return Debug::Method::PRIM_ID;
  else if (name == "geomID")
    return Debug::Method::GEOM_ID;
  else if (name == "instID")
    return Debug::Method::INST_ID;
  else if (name == "Ng")
    return Debug::Method::NG;
  else if (name == "Ng.abs")
    return Debug::Method::NG_ABS;
  else if (name == "Ns")
    return Debug::Method::NS;
  else if (name == "Ns.abs")
    return Debug::Method::NS_ABS;
  else if (name == "uvw")
    return Debug::Method::RAY_UVW;
  else if (name == "istri")
    return Debug::Method::IS_TRIANGLE;
  else if (name == "isvol")
    return Debug::Method::IS_VOLUME;
  else
    return Debug::Method::BACKFACE;
}

void Debug::commit()
{
  Renderer::commit();
  m_method = methodFromString(getParam<std::string>("method", "primID"));
}

void Debug::populateFrameData(FrameGPUData &fd) const
{
  Renderer::populateFrameData(fd);
  fd.renderer.params.debug.method = static_cast<int>(m_method);
}

OptixModule Debug::optixModule() const
{
  return deviceState()->rendererModules.debug;
}

anari::Span<const HitgroupFunctionNames> Debug::hitgroupSbtNames() const
{
  return anari::make_Span(g_debugHitNames.data(), g_debugHitNames.size());
}

anari::Span<const std::string> Debug::missSbtNames() const
{
  return anari::make_Span(g_debugMissNames.data(), g_debugMissNames.size());
}

ptx_ptr Debug::ptx()
{
  return Debug_ptx;
}

} // namespace visrtx
