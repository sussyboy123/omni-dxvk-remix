# D3D11 Phase 1: Shader-Aware Reconstruction Foundation

This document defines the first implementation phase for introducing D3D11 support with Shader-Aware Reconstruction into this dxvk-remix fork.

## Goals

- Capture enough D3D11 frame metadata to reconstruct scene geometry and material intent.
- Normalize captured metadata into a renderer-agnostic intermediate representation (IR).
- Bridge IR records into `src/dxvk/rtx_render/` without disrupting existing D3D9-era behavior.
- Provide deterministic debug visibility to validate semantic inference quality.

## Non-goals

- Full D3D12 support (covered in later phases).
- Complete game-agnostic PBR perfection.
- ML inference integration (optional future enhancement).

## Scope

Phase 1 should be constrained to:

1. **D3D11 draw/resource metadata capture**.
2. **Shader/material semantic inference (rules-based, confidence scored)**.
3. **IR creation and caching**.
4. **IR-to-RTX bridge integration behind a runtime option gate**.
5. **Developer diagnostics and validation tooling**.

## Proposed Architecture

## 1) D3D11 Capture Layer

Add a capture pipeline near D3D11 draw submission to collect:

- Shader bytecode hashes (`VS/PS`, optional `GS/HS/DS/CS` identifiers).
- Bound SRV/UAV/CBV slot usage per stage.
- Input layout signature and vertex/index buffer formats/strides.
- Constant buffer snapshots for transform/material candidate extraction.
- Resource dimensions and format metadata for texture classification.

### Suggested data model

```cpp
struct D3d11DrawCapture {
  uint64_t frameId;
  uint64_t drawId;
  uint64_t pipelineHash;
  uint64_t vsHash;
  uint64_t psHash;
  InputAssemblySignature ia;
  ResourceBindingSnapshot bindings;
  TransformCandidateData transforms;
};
```

## 2) Shader-Aware Semantic Inference

Build a deterministic inference module that maps draw captures to semantic labels:

- `baseColor`, `normal`, `roughnessMetallic`, `emissive`, `opacityMask` candidates.
- Object transform source confidence.
- Skinned vs static mesh classification.
- Alpha mode classification (`opaque`, `masked`, `blended`).

### Confidence scoring

Each inferred semantic should carry:

- `confidence` (`0.0f` to `1.0f`)
- `reasonFlags` (bitmask for rule traces)

This preserves explainability and enables conservative fallback behavior.

## 3) Unified Reconstruction IR

Create a shared IR for RTX ingestion independent of API frontend.

```cpp
struct ReconstructionMaterialSemantic {
  ResourceHandle baseColor;
  ResourceHandle normal;
  ResourceHandle roughnessMetallic;
  ResourceHandle emissive;
  float baseColorConfidence;
  float normalConfidence;
  uint32_t reasonFlags;
};

struct ReconstructionDrawRecord {
  uint64_t frameId;
  uint64_t drawId;
  uint64_t meshSignature;
  Matrix4 objectToWorld;
  ReconstructionMaterialSemantic material;
  AlphaMode alphaMode;
};
```

### Caching strategy

- Shader signature cache key: `(vsHash, psHash, inputLayoutSignature, resourcePatternHash)`.
- Cache inferred semantics for reuse across draws/frames.
- Invalidate on shader reload or resource layout mismatch.

## 4) RTX Bridge Integration

Add an adapter layer that transforms `ReconstructionDrawRecord` into internal RTX scene updates.

Requirements:

- Runtime gate via an RTX option (for gradual rollout).
- Non-invasive integration path that preserves legacy behavior when disabled.
- CPU profiling zones around capture/inference/bridge steps.

## 5) Debug & Validation

Add developer-facing diagnostics:

- Overlay panel listing inferred semantics per selected draw.
- Confidence heatmap mode.
- Logging mode for captures and inference traces.
- Optional dump-to-disk for deterministic replay analysis.

## Milestones

### M1: Capture skeleton

- Collect draw/pipeline/shader hashes and IA signatures.
- Emit structured logs for a sampled subset of draws.

### M2: Inference v1

- Implement deterministic semantic rules.
- Emit confidences and reason flags.

### M3: IR + cache

- Introduce IR structs and caching layer.
- Validate frame-to-frame semantic stability.

### M4: RTX adapter + option gate

- Feed IR into RTX path behind an option.
- Verify no behavior change when option disabled.

### M5: Debug tooling

- Add overlay and trace output.
- Validate on at least one known D3D11 title.

## Testing checklist (Phase 1)

- Unit tests for inference rules and confidence calculations.
- Regression tests for cache key stability.
- Smoke tests verifying option-gated enable/disable behavior.
- Performance check for capture overhead and cache hit rate.

## Risks and mitigations

- **Semantic ambiguity**: Use confidence thresholds and fallbacks.
- **Performance overhead**: Sample draws initially, introduce caching early.
- **False positives on material mapping**: Preserve conservative defaults and debug traces.
- **Integration churn**: Keep adapter boundary strict and API-agnostic.

## Exit criteria

Phase 1 is complete when:

1. D3D11 draws are captured and transformed into IR.
2. Inference produces stable semantics with confidence metrics.
3. IR records can be consumed by RTX bridge under a runtime gate.
4. Developers can inspect, trace, and validate inference output.
