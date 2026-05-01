/*
* Copyright (c) 2021-2026, NVIDIA CORPORATION. All rights reserved.
*/
#pragma once

#include <cstdint>
#include <optional>
#include <unordered_map>

#include "../util/util_matrix.h"

namespace dxvk {

  enum class ReconstructionAlphaMode : uint8_t {
    Opaque = 0,
    Masked = 1,
    Blended = 2,
  };

  struct D3d11DrawCapture {
    uint64_t frameId = 0;
    uint64_t drawId = 0;
    uint64_t pipelineHash = 0;
    uint64_t vsHash = 0;
    uint64_t psHash = 0;
    uint64_t inputLayoutHash = 0;
    uint64_t resourcePatternHash = 0;
    bool blendEnabled = false;
  };

  struct ReconstructionMaterialSemantic {
    float baseColorConfidence = 0.0f;
    float normalConfidence = 0.0f;
    float roughnessMetallicConfidence = 0.0f;
    float emissiveConfidence = 0.0f;
    uint32_t reasonFlags = 0;
  };

  struct ReconstructionDrawRecord {
    uint64_t frameId = 0;
    uint64_t drawId = 0;
    uint64_t meshSignature = 0;
    Matrix4 objectToWorld = Matrix4();
    ReconstructionMaterialSemantic material;
    ReconstructionAlphaMode alphaMode = ReconstructionAlphaMode::Opaque;
  };

  class ShaderAwareReconstruction {
  public:
    struct CacheKey {
      uint64_t vsHash = 0;
      uint64_t psHash = 0;
      uint64_t inputLayoutHash = 0;
      uint64_t resourcePatternHash = 0;

      bool operator==(const CacheKey& other) const {
        return vsHash == other.vsHash
            && psHash == other.psHash
            && inputLayoutHash == other.inputLayoutHash
            && resourcePatternHash == other.resourcePatternHash;
      }
    };

    struct CacheKeyHash {
      size_t operator()(const CacheKey& key) const;
    };

    ReconstructionDrawRecord inferOrCreate(const D3d11DrawCapture& drawCapture);
    void clearCache();
    size_t cacheSize() const;

  private:
    ReconstructionDrawRecord infer(const D3d11DrawCapture& drawCapture) const;
    CacheKey makeCacheKey(const D3d11DrawCapture& drawCapture) const;

    std::unordered_map<CacheKey, ReconstructionMaterialSemantic, CacheKeyHash> m_materialCache;
  };

} // namespace dxvk
