#pragma once
// JOCKY registry metadata — ScriptMetadata struct and scanner interface
// (Phase 2). The scanner (src/stdlib/metadata_scanner.cpp) walks a
// directory tree, parses `@jocky:` headers, validates them, and returns
// a ScanResult. Field-for-field contract for the struct is fixed by the
// Phase 2 session brief; validation rules mirror wiki/06-api-contracts.md
// §4 with the documented reconciliations (see metadata_scanner.cpp).

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace jocky {

struct ScriptMetadata {
    std::string function;
    std::string domain;
    std::string description;
    // Ordered (name, type) pairs; the "= default" suffix, if present in
    // the header, is stripped from the stored type.
    std::vector<std::pair<std::string, std::string>> inputs;
    // The type part of `outputs <name>: <type>`; the whole value when no
    // colon is present (accepts both `table<flow>` and bare `csv` forms).
    std::string output_type;
    std::string capability;
    int timeout_seconds = 30;  // default when the header omits the key
    std::vector<std::string> depends_on;
    std::string script_path;  // as scanned (relative when root is relative)
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
