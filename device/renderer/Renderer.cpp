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

#include "Renderer.h"
#include "ParameterInfo.h"
// specific renderers
#include "AmbientOcclusion.h"
#include "Debug.h"
#include "DiffusePathTracer.h"
#include "Raycast.h"
#include "SciVis.h"
// std
#include <stdlib.h>
#include <string_view>
// this include may only appear in a single source file:
#include <optix_function_table_definition.h>

namespace visrtx {

struct SBTRecord
{
  alignas(OPTIX_SBT_RECORD_ALIGNMENT) char header[OPTIX_SBT_RECORD_HEADER_SIZE];
};

using RaygenRecord = SBTRecord;
using MissRecord = SBTRecord;
using HitgroupRecord = SBTRecord;

// Helper functions ///////////////////////////////////////////////////////////

static Renderer *make_renderer(std::string_view subtype)
{
  auto splitString = [](const std::string &input,
                         const std::string &delim) -> std::vector<std::string> {
    std::vector<std::string> tokens;
    size_t pos = 0;
    while (true) {
      size_t begin = input.find_first_not_of(delim, pos);
      if (begin == input.npos)
        return tokens;
      size_t end = input.find_first_of(delim, begin);
      tokens.push_back(input.substr(
          begin, (end == input.npos) ? input.npos : (end - begin)));
      pos = end;
    }
  };

  Renderer *retval = nullptr;

  if (subtype == "raycast")
    retval = new Raycast();
  else if (subtype == "ao")
    retval = new AmbientOcclusion();
  else if (subtype == "diffuse_pathtracer" || subtype == "dpt")
    retval = new DiffusePathTracer();
  else if (subtype == "scivis" || subtype == "sv" || subtype == "default")
    retval = new SciVis();
  else {
    retval = new Debug();
    auto names = splitString(std::string(subtype), "_");
    if (names.size() > 1)
      retval->setParam("method", names[1]);
  }

  return retval;
}

static std::string longestBeginningMatch(
    const std::string_view &first, const std::string_view &second)
{
  auto maxMatchLength = std::min(first.size(), second.size());
  auto start1 = first.begin();
  auto start2 = second.begin();
  auto end = first.begin() + maxMatchLength;

  return std::string(start1, std::mismatch(start1, end, start2).first);
}

static bool beginsWith(const std::string_view &inputString,
    const std::string_view &startsWithString)
{
  auto startingMatch = longestBeginningMatch(inputString, startsWithString);
  return startingMatch.size() == startsWithString.size();
}

// Renderer definitions ///////////////////////////////////////////////////////

static size_t s_numRenderers = 0;

size_t Renderer::objectCount()
{
  return s_numRenderers;
}

Renderer::Renderer()
{
  s_numRenderers++;
}

Renderer::~Renderer()
{
  optixPipelineDestroy(m_pipeline);
  s_numRenderers--;
}

void Renderer::commit()
{
  m_bgColor = getParam<vec4>("backgroundColor", vec4(1.f));
  m_spp = getParam<int>("pixelSamples", 1);
}

anari::Span<const HitgroupFunctionNames> Renderer::hitgroupSbtNames() const
{
  return anari::make_Span(&m_defaultHitgroupNames, 1);
}

anari::Span<const std::string> Renderer::missSbtNames() const
{
  return anari::make_Span(&m_defaultMissName, 1);
}

void Renderer::populateFrameData(FrameGPUData &fd) const
{
  fd.renderer.bgColor = m_bgColor;
}

OptixPipeline Renderer::pipeline() const
{
  return m_pipeline;
}

const OptixShaderBindingTable *Renderer::sbt()
{
  if (!m_pipeline)
    initOptixPipeline();

  return &m_sbt;
}

vec4 Renderer::bgColor() const
{
  return m_bgColor;
}

int Renderer::spp() const
{
  return m_spp;
}

Renderer *Renderer::createInstance(
    std::string_view subtype, DeviceGlobalState *d)
{
  Renderer *retval = nullptr;

  auto *overrideType = getenv("VISRTX_OVERRIDE_RENDERER");

  if (overrideType != nullptr)
    subtype = overrideType;

  retval = make_renderer(subtype);

  retval->setDeviceState(d);
  return retval;
}

const ANARIParameter *Renderer::getParameters(std::string_view subtype)
{
  ANARIParameter emptyParam = {nullptr, ANARI_UNKNOWN};

  if (subtype == "raycast") {
    static const ANARIParameter raycast[] = {
        {"backgroundColor", ANARI_FLOAT32_VEC4},
        {"pixelSamples", ANARI_INT32},
        emptyParam};
    return raycast;
  } else if (subtype == "ao") {
    static const ANARIParameter ao[] = {{"backgroundColor", ANARI_FLOAT32_VEC4},
        {"pixelSamples", ANARI_INT32},
        {"aoSamples", ANARI_INT32},
        emptyParam};
    return ao;
  } else if (subtype == "diffuse_pathtracer" || subtype == "dpt") {
    static const ANARIParameter dpt[] = {
        {"backgroundColor", ANARI_FLOAT32_VEC4},
        {"pixelSamples", ANARI_INT32},
        {"maxDepth", ANARI_INT32},
        {"R", ANARI_FLOAT32},
        emptyParam};
    return dpt;
  } else if (subtype == "scivis" || subtype == "sv" || subtype == "default") {
    static const ANARIParameter scivis[] = {
        {"backgroundColor", ANARI_FLOAT32_VEC4},
        {"pixelSamples", ANARI_INT32},
        {"lightFalloff", ANARI_FLOAT32},
        {"ambientSamples", ANARI_INT32},
        {"ambientIntensity", ANARI_FLOAT32},
        {"ambientColor", ANARI_FLOAT32_VEC3},
        emptyParam};
    return scivis;
  } else if (beginsWith(subtype, "debug")) {
    static const ANARIParameter method[] = {
        {"backgroundColor", ANARI_FLOAT32_VEC4},
        {"pixelSamples", ANARI_INT32},
        {"method", ANARI_STRING},
        emptyParam};
    return method;
  }

  return nullptr;
}

const void *Renderer::getParameterInfo(std::string_view subtype,
    std::string_view paramName,
    ANARIDataType paramType,
    std::string_view infoName,
    ANARIDataType infoType)
{
  if (paramName == "backgroundColor" && paramType == ANARI_FLOAT32) {
    static const ParameterInfo param(false, "background color", vec4(1.f));
    return param.fromString(infoName, infoType);
  } else if (paramName == "pixelSamples" && paramType == ANARI_INT32) {
    static const ParameterInfo param(false, "samples per-pixel each frame", 1);
    return param.fromString(infoName, infoType);
  } else {
    if (subtype == "ao") {
      AmbientOcclusion::getParameterInfo(
          paramName, paramType, infoName, infoType);
    } else if (subtype == "diffuse_pathtracer" || subtype == "dpt") {
      DiffusePathTracer::getParameterInfo(
          paramName, paramType, infoName, infoType);
    } else if (subtype == "scivis" || subtype == "sv" || subtype == "default") {
      SciVis::getParameterInfo(paramName, paramType, infoName, infoType);
    } else if (beginsWith(subtype, "debug")) {
      // Nothing to report
    }
  }

  return nullptr;
}

void Renderer::initOptixPipeline()
{
  auto &state = *deviceState();

  auto om = optixModule();

  char log[2048];
  size_t sizeof_log = sizeof(log);

  // Raygen program //

  {
    m_raygenPGs.resize(1);

    OptixProgramGroupOptions pgOptions = {};
    OptixProgramGroupDesc pgDesc = {};
    pgDesc.kind = OPTIX_PROGRAM_GROUP_KIND_RAYGEN;
    pgDesc.raygen.module = om;
    pgDesc.raygen.entryFunctionName = "__raygen__";

    sizeof_log = sizeof(log);
    OPTIX_CHECK(optixProgramGroupCreate(state.optixContext,
        &pgDesc,
        1,
        &pgOptions,
        log,
        &sizeof_log,
        &m_raygenPGs[0]));

    if (sizeof_log > 1)
      reportMessage(ANARI_SEVERITY_DEBUG, "PG Raygen Log:\n%s\n", log);
  }

  // Miss program //

  {
    auto missNames = missSbtNames();

    m_missPGs.resize(missNames.size());

    for (int i = 0; i < m_missPGs.size(); i++) {
      auto &mn = missNames[i];

      OptixProgramGroupOptions pgOptions = {};
      OptixProgramGroupDesc pgDesc = {};
      pgDesc.kind = OPTIX_PROGRAM_GROUP_KIND_MISS;
      pgDesc.miss.module = om;
      pgDesc.miss.entryFunctionName = mn.c_str();

      sizeof_log = sizeof(log);
      OPTIX_CHECK(optixProgramGroupCreate(state.optixContext,
          &pgDesc,
          1,
          &pgOptions,
          log,
          &sizeof_log,
          &m_missPGs[i]));

      if (sizeof_log > 1)
        reportMessage(ANARI_SEVERITY_DEBUG, "PG Miss Log:\n%s", log);
    }
  }

  // Hit program //

  {
    auto hitgroupNames = hitgroupSbtNames();

    m_hitgroupPGs.resize(hitgroupNames.size());

    for (int i = 0; i < m_hitgroupPGs.size(); i++) {
      auto &hgn = hitgroupNames[i];

      OptixProgramGroupOptions pgOptions = {};
      OptixProgramGroupDesc pgDesc = {};
      pgDesc.kind = OPTIX_PROGRAM_GROUP_KIND_HITGROUP;

      pgDesc.hitgroup.moduleCH = om;
      pgDesc.hitgroup.entryFunctionNameCH = hgn.closestHit.c_str();

      if (!hgn.anyHit.empty()) {
        pgDesc.hitgroup.moduleAH = om;
        pgDesc.hitgroup.entryFunctionNameAH = hgn.anyHit.c_str();
      }

      pgDesc.hitgroup.moduleIS = state.intersectionModules.customIntersectors;
      pgDesc.hitgroup.entryFunctionNameIS = "__intersection__";

      sizeof_log = sizeof(log);
      OPTIX_CHECK(optixProgramGroupCreate(state.optixContext,
          &pgDesc,
          1,
          &pgOptions,
          log,
          &sizeof_log,
          &m_hitgroupPGs[i]));

      if (sizeof_log > 1)
        reportMessage(ANARI_SEVERITY_DEBUG, "PG Hitgroup Log:\n%s", log);
    }
  }

  // Pipeline //

  {
    OptixPipelineCompileOptions pipelineCompileOptions = {};
    pipelineCompileOptions.traversableGraphFlags =
        OPTIX_TRAVERSABLE_GRAPH_FLAG_ALLOW_SINGLE_LEVEL_INSTANCING;
    pipelineCompileOptions.usesMotionBlur = false;
    pipelineCompileOptions.numPayloadValues = PAYLOAD_VALUES;
    pipelineCompileOptions.numAttributeValues = ATTRIBUTE_VALUES;
    pipelineCompileOptions.exceptionFlags = OPTIX_EXCEPTION_FLAG_NONE;
    pipelineCompileOptions.pipelineLaunchParamsVariableName = "frameData";

    OptixPipelineLinkOptions pipelineLinkOptions = {};
    pipelineLinkOptions.maxTraceDepth = 2;

    std::vector<OptixProgramGroup> programGroups;
    for (auto pg : m_raygenPGs)
      programGroups.push_back(pg);
    for (auto pg : m_missPGs)
      programGroups.push_back(pg);
    for (auto pg : m_hitgroupPGs)
      programGroups.push_back(pg);

    sizeof_log = sizeof(log);
    OPTIX_CHECK(optixPipelineCreate(state.optixContext,
        &pipelineCompileOptions,
        &pipelineLinkOptions,
        programGroups.data(),
        programGroups.size(),
        log,
        &sizeof_log,
        &m_pipeline));

    if (sizeof_log > 1)
      reportMessage(ANARI_SEVERITY_DEBUG, "Pipeline Create Log:\n%s", log);
  }

  // SBT //

  {
    std::vector<RaygenRecord> raygenRecords;
    for (auto &pg : m_raygenPGs) {
      RaygenRecord rec;
      OPTIX_CHECK(optixSbtRecordPackHeader(pg, &rec));
      raygenRecords.push_back(rec);
    }
    m_raygenRecordsBuffer.upload(raygenRecords);
    m_sbt.raygenRecord = (CUdeviceptr)m_raygenRecordsBuffer.ptr();

    std::vector<MissRecord> missRecords;
    for (auto &pg : m_missPGs) {
      MissRecord rec;
      OPTIX_CHECK(optixSbtRecordPackHeader(pg, &rec));
      missRecords.push_back(rec);
    }
    m_missRecordsBuffer.upload(missRecords);
    m_sbt.missRecordBase = (CUdeviceptr)m_missRecordsBuffer.ptr();
    m_sbt.missRecordStrideInBytes = sizeof(MissRecord);
    m_sbt.missRecordCount = missRecords.size();

    std::vector<HitgroupRecord> hitgroupRecords;
    for (auto &hpg : m_hitgroupPGs) {
      HitgroupRecord rec;
      OPTIX_CHECK(optixSbtRecordPackHeader(hpg, &rec));
      hitgroupRecords.push_back(rec);
    }
    m_hitgroupRecordsBuffer.upload(hitgroupRecords);
    m_sbt.hitgroupRecordBase = (CUdeviceptr)m_hitgroupRecordsBuffer.ptr();
    m_sbt.hitgroupRecordStrideInBytes = sizeof(HitgroupRecord);
    m_sbt.hitgroupRecordCount = hitgroupRecords.size();
  }
}

} // namespace visrtx

VISRTX_ANARI_TYPEFOR_DEFINITION(visrtx::Renderer *);
