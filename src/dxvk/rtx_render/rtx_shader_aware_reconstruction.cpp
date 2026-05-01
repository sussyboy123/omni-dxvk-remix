/*
* Copyright (c) 2021-2026, NVIDIA CORPORATION. All rights reserved.
*/

#include "rtx_shader_aware_reconstruction.h"

namespace dxvk {

  size_t ShaderAwareReconstruction::CacheKeyHash::operator()(const CacheKey& key) const {
    const size_t h1 = std::hash<uint64_t>{}(key.vsHash);
    const size_t h2 = std::hash<uint64_t>{}(key.psHash);
    const size_t h3 = std::hash<uint64_t>{}(key.inputLayoutHash);
    const size_t h4 = std::hash<uint64_t>{}(key.resourcePatternHash);
    return ((h1 ^ (h2 << 1)) >> 1) ^ (h3 << 1) ^ (h4 << 2);
  }

  ReconstructionDrawRecord ShaderAwareReconstruction::inferOrCreate(const D3d11DrawCapture& drawCapture) {
    const CacheKey key = makeCacheKey(drawCapture);

    ReconstructionDrawRecord drawRecord;
    drawRecord.frameId = drawCapture.frameId;
    drawRecord.drawId = drawCapture.drawId;
    drawRecord.meshSignature = drawCapture.pipelineHash;

    const auto found = m_materialCache.find(key);
    if (found != m_materialCache.end()) {
      drawRecord.material = found->second;
      drawRecord.alphaMode = drawCapture.blendEnabled ? ReconstructionAlphaMode::Blended : ReconstructionAlphaMode::Opaque;
      return drawRecord;
    }

    drawRecord = infer(drawCapture);
    m_materialCache.emplace(key, drawRecord.material);
    return drawRecord;
  }

  void ShaderAwareReconstruction::clearCache() {
    m_materialCache.clear();
  }

  size_t ShaderAwareReconstruction::cacheSize() const {
    return m_materialCache.size();
  }

  ReconstructionDrawRecord ShaderAwareReconstruction::infer(const D3d11DrawCapture& drawCapture) const {
    ReconstructionDrawRecord drawRecord;

    drawRecord.frameId = drawCapture.frameId;
    drawRecord.drawId = drawCapture.drawId;
    drawRecord.meshSignature = drawCapture.pipelineHash;

    // Phase-1 deterministic bootstrap: the confidence values below are intentionally
    // conservative and are designed to be replaced by richer shader/resource rules.
    drawRecord.material.baseColorConfidence = 0.5f;
    drawRecord.material.normalConfidence = 0.2f;
    drawRecord.material.roughnessMetallicConfidence = 0.1f;
    drawRecord.material.emissiveConfidence = 0.05f;
    drawRecord.material.reasonFlags = 0x1u;

    drawRecord.alphaMode = drawCapture.blendEnabled ? ReconstructionAlphaMode::Blended : ReconstructionAlphaMode::Opaque;

    return drawRecord;
  }

  ShaderAwareReconstruction::CacheKey ShaderAwareReconstruction::makeCacheKey(const D3d11DrawCapture& drawCapture) const {
    CacheKey key;
    key.vsHash = drawCapture.vsHash;
    key.psHash = drawCapture.psHash;
    key.inputLayoutHash = drawCapture.inputLayoutHash;
    key.resourcePatternHash = drawCapture.resourcePatternHash;
    return key;
  }

} // namespace dxvk
