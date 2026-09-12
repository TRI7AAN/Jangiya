#pragma once
// JOCKY registry metadata — ScriptMetadata struct and scanner interface
// (Phase 2; input defaults stored since Phase 3). The scanner
// (src/stdlib/metadata_scanner.cpp) walks a directory tree, parses
// `@jocky:` headers, validates them, and returns a ScanResult.
// Field-for-field contract for the struct is fixed by the Phase 2
// session brief as amended by the Phase 3 `= default` decision (see
// wiki/06-api-contracts.md §4 and wiki/12-phase3-resolution.md);
// validation rules mirror those docs with the documented
// reconciliations (see metadata_scanner.cpp).

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace jocky {

struct InputParam {
    std::string name;
    // Declared type with any `= default` suffix stripped.
    std::string type;
    // True when the header entry carried `= <default>` (Phase 3 `= default`
    // decision: such inputs may be omitted at a call site, with
    // default_value filling in). Quotes around the raw default are
    // stripped; the stored value is the trimmed inner text.
    bool has_default = false;
    std::string default_value;
};

struct ScriptMetadata {
    std::string function;
    std::string domain;
    std::string description;
    // Ordered inputs; see InputParam.
    std::vector<InputParam> inputs;
    // The type part of `outputs <name>: <type>`; the whole value when no
    // colon is present (accepts both `table<flow>` and bare `csv` forms).
    std::string output_type;
    std::string capability;
    int timeout_seconds = 30;  // default when the header omits the key
    std::vector<std::string> depends_on;
    std::string script_path;  // as scanned (relative when root is relative)
    // SHA-256 captured when the registry entry was scanned. Phase 6 rechecks
    // it immediately before embedding to refuse source drift.
    std::string source_sha256;
};

struct ScanResult {
    std::vector<ScriptMetadata> registry;  // sorted by function name
    std::vector<std::string> rejections;  // "path: reason" (exit non-zero)
    std::vector<std::string> skipped;     // "path: reason" (informational)
    std::map<std::string, int> per_domain_counts;  // all six domains present
};

// Recursively scan `root` for *.sh files. Directories named
// `_quarantine` are skipped at any depth; missing or empty domain
// folders are valid (zero registered functions), not errors.
ScanResult scan_registry_dir(const std::string& root);

// Serialize the registry (functions only) as JSON to stdout.
std::string registry_to_json(const ScanResult& result);

}  // namespace jocky
