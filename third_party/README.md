# Vendored third-party dependencies

PERSIST-01 vendors persistence-format dependencies so normal configure/build/test does not fetch code from the network.

## miniz
- Upstream: richgel999/miniz
- Version: 3.1.2 (2026-07-01)
- License: included in `third_party/miniz-3.1.2/LICENSE`
- Purpose: ZIP-compatible native Document container read/write.

## JSON for Modern C++
- Upstream: nlohmann/json
- Version: 3.12.0 (2025-04-11)
- License: MIT, included in `third_party/nlohmann-json-3.12.0/LICENSE.MIT`
- Purpose: UTF-8 JSON parsing/serialization for native Document manifest and domain-owned authored payloads.

These libraries are persistence implementation details and must not leak into CAD-domain public semantic APIs.
