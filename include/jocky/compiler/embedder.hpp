#pragma once
// Phase 6 embedder: binds a shaken ScriptMetadata list to immutable source
// bytes. The scan-time digest is rechecked immediately before generation.

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "jocky/crypto/sha256.hpp"
#include "jocky/fir/script_resolver.hpp"

namespace jocky {

class EmbedError : public std::runtime_error {
public:
    explicit EmbedError(const std::string& message)
        : std::runtime_error(message) {}
};

struct EmbeddedScript {
    ScriptMetadata metadata;
    std::string source;
    std::string sha256;
};

struct EmbedResult {
    std::vector<EmbeddedScript> scripts;
};

inline EmbedResult embed_scripts(const ShakeResult& shaken) {
    EmbedResult result;
    result.scripts.reserve(shaken.scripts.size());
    for (const ResolvedScript& resolved : shaken.scripts) {
        const ScriptMetadata& metadata = resolved.metadata;
        if (metadata.source_sha256.empty()) {
            throw EmbedError("script '" + metadata.function +
                             "' has no scan-time SHA-256");
        }
        std::string source;
        try {
            source = read_binary_file(metadata.script_path);
        } catch (const std::exception& ex) {
            throw EmbedError("cannot embed '" + metadata.function +
                             "': " + ex.what());
        }
        const std::string digest = sha256_bytes(source);
        if (digest != metadata.source_sha256) {
            throw EmbedError(
                "script changed after registry scan: '" + metadata.function +
                "' expected " + metadata.source_sha256 + " but found " +
                digest + " (refusing stale embed)");
        }
        result.scripts.push_back(
            EmbeddedScript{metadata, std::move(source), digest});
    }
    return result;
}

}  // namespace jocky
