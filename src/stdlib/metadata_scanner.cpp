// JOCKY registry scanner implementation (Phase 2).
//
// Header contract (wiki/06-api-contracts.md §4) with reconciliations:
//   - inputs entries are `name: type` with an optional `= default`
//     suffix (Phase 3 decision: the default is STORED on InputParam, not
//     dropped — call sites may omit defaulted inputs, and the scanner
//     validates the default against the declared type at scan time).
//     A present-but-empty default (`name: type =`) is a rejection.
//     One pair of surrounding double quotes is stripped from the stored
//     default (`bpf: string = ""` stores an empty default, which still
//     counts as defaulted).
//   - outputs is `name: type`; only the type part is stored in
//     output_type. A bare value without a colon (e.g. `csv`) is stored
//     whole, so both the typed (`table<flow>`) and format (`json|csv|
//     text`) spellings parse.
//   - timeout_seconds missing -> default 30 (struct default); present
//     but non-positive/non-numeric -> rejection.
//   - depends_on splits on commas and/or whitespace; absent -> none.
//   - function/domain/description/inputs/outputs/capability must all be
//     present and non-empty; unknown `@jocky:` keys and duplicate keys
//     are rejections (typo detection is the point of a validator).
// Folder rule: a file under one of the six domain directories must
// declare that same domain. Files under shared/ (cross-domain helpers)
// or directly at the scan root are validated without the folder check.

#include "jocky/stdlib/script_metadata.hpp"
#include "jocky/crypto/sha256.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace jocky {
namespace fs = std::filesystem;

namespace {

const std::vector<std::string>& valid_domains() {
    static const std::vector<std::string> domains = {
        "recon",      "netforensics", "hostforensics",
        "timeline",   "compliance",   "report",
    };
    return domains;
}

bool is_domain(const std::string& value) {
    const auto& domains = valid_domains();
    return std::find(domains.begin(), domains.end(), value) != domains.end();
}

std::string trim(const std::string& text) {
    std::size_t begin = 0;
    while (begin < text.size() &&
           std::isspace(static_cast<unsigned char>(text[begin]))) {
        ++begin;
    }
    std::size_t end = text.size();
    while (end > begin &&
           std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }
    return text.substr(begin, end - begin);
}

// Split on commas, honoring double-quoted sections (defaults may
// contain commas inside quotes).
std::vector<std::string> split_list(const std::string& text) {
    std::vector<std::string> parts;
    std::string current;
    bool in_quotes = false;
    for (char c : text) {
        if (c == '"') {
            in_quotes = !in_quotes;
            current += c;
        } else if (c == ',' && !in_quotes) {
            parts.push_back(trim(current));
            current.clear();
        } else {
            current += c;
        }
    }
    if (!trim(current).empty() || !parts.empty()) {
        parts.push_back(trim(current));
    }
    return parts;
}

std::vector<std::string> split_words(const std::string& text) {
    std::vector<std::string> words;
    std::istringstream stream(text);
    std::string word;
    while (stream >> word) {
        words.push_back(word);
    }
    return words;
}

bool is_lower_alnum(const std::string& text) {
    if (text.empty()) {
        return false;
    }
    for (char c : text) {
        const unsigned char u = static_cast<unsigned char>(c);
        if (!std::islower(u) && !std::isdigit(u)) {
            return false;
        }
    }
    return true;
}

bool is_lower_alnum_us(const std::string& text) {
    if (text.empty()) {
        return false;
    }
    for (char c : text) {
        const unsigned char u = static_cast<unsigned char>(c);
        if (!std::islower(u) && !std::isdigit(u) && c != '_') {
            return false;
        }
    }
    return true;
}

// Strict check for jky_<domain>_<verb>_<object>; on success sets
// name_domain to the embedded domain segment.
bool check_function_name(const std::string& name, std::string& name_domain) {
    static const std::string prefix = "jky_";
    if (name.compare(0, prefix.size(), prefix) != 0) {
        return false;
    }
    const std::string rest = name.substr(prefix.size());
    for (const std::string& domain : valid_domains()) {
        const std::string head = domain + "_";
        if (rest.compare(0, head.size(), head) == 0) {
            const std::string tail = rest.substr(head.size());
            const std::size_t sep = tail.find('_');
            if (sep == std::string::npos || sep == 0 ||
                sep + 1 >= tail.size()) {
                return false;
            }
            if (!is_lower_alnum(tail.substr(0, sep))) {
                return false;
            }
            if (!is_lower_alnum_us(tail.substr(sep + 1))) {
                return false;
            }
            name_domain = domain;
            return true;
        }
    }
    return false;
}

std::string json_escape(const std::string& text) {
    std::string out;
    for (char c : text) {
        const unsigned char u = static_cast<unsigned char>(c);
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (u < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", u);
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    return out;
}

// Phase 3: does a stored `= default` value satisfy its declared input
// type? string/path accept anything (paths travel as string literals);
// int wants an optional sign plus digits; float accepts that plus a
// decimal/exponent form; bool wants exactly true/false. Unknown type
// names never match, so undeclared input types are rejected at scan
// time rather than surfacing in the resolver.
bool default_matches_type(const std::string& value, const std::string& type) {
    if (type == "string" || type == "path") {
        return true;
    }
    if (type == "bool") {
        return value == "true" || value == "false";
    }
    if (type != "int" && type != "float") {
        return false;
    }
    std::size_t i = 0;
    if (i < value.size() && (value[i] == '+' || value[i] == '-')) {
        ++i;
    }
    bool digits_before = false;
    while (i < value.size() &&
           std::isdigit(static_cast<unsigned char>(value[i]))) {
        ++i;
        digits_before = true;
    }
    if (!digits_before) {
        return false;
    }
    if (i == value.size()) {
        return true;  // plain integer form; also valid for float
    }
    if (type == "int") {
        return false;
    }
    if (value[i] == '.') {  // fractional part
        ++i;
        bool digits_after = false;
        while (i < value.size() &&
               std::isdigit(static_cast<unsigned char>(value[i]))) {
            ++i;
            digits_after = true;
        }
        if (!digits_after) {
            return false;
        }
    }
    if (i < value.size() && (value[i] == 'e' || value[i] == 'E')) {
        ++i;
        if (i < value.size() && (value[i] == '+' || value[i] == '-')) {
            ++i;
        }
        bool digits_exp = false;
        while (i < value.size() &&
               std::isdigit(static_cast<unsigned char>(value[i]))) {
            ++i;
            digits_exp = true;
        }
        if (!digits_exp) {
            return false;
        }
    }
    return i == value.size();
}

struct ParsedFile {
    std::string path;
    std::string folder_domain;  // "" when not directly under a domain dir
    std::map<std::string, std::string> keys;
    bool has_any_header = false;
};

}  // namespace

ScanResult scan_registry_dir(const std::string& root) {
    ScanResult result;
    for (const std::string& domain : valid_domains()) {
        result.per_domain_counts[domain] = 0;
    }

    std::vector<ParsedFile> parsed;
    // Collect candidate paths first, then parse in sorted order so that
    // duplicate-name resolution (first seen wins) is deterministic across
    // filesystems and runs. Reproducible registry output matters for the
    // compiled/interpreted manifest parity requirement.
    std::vector<std::string> candidate_paths;
    std::error_code ec;
    fs::recursive_directory_iterator it(root, ec), end;
    if (ec) {
        result.rejections.push_back(root + ": cannot scan directory (" +
                                    ec.message() + ")");
        return result;
    }
    for (; it != end; it.increment(ec)) {
        if (ec) {
            result.rejections.push_back(root + ": traversal error (" +
                                        ec.message() + ")");
            break;
        }
        const fs::path path = it->path();
        bool is_dir = it->is_directory(ec);
        if (ec) {
            ec.clear();
            continue;
        }
        if (is_dir) {
            if (path.filename() == "_quarantine") {
                it.disable_recursion_pending();
            }
            continue;
        }
        if (path.extension() != ".sh") {
            continue;
        }
        const std::string filename = path.filename().string();
        if (!filename.empty() && filename[0] == '.') {
            continue;
        }
        candidate_paths.push_back(path.generic_string());
    }
    std::sort(candidate_paths.begin(), candidate_paths.end());
    for (const std::string& script_path : candidate_paths) {
        const fs::path path(script_path);
        ParsedFile file;
        file.path = script_path;
        // Folder domain: first path component under the scan root.
        fs::path rel = fs::relative(path, root, ec);
        if (!ec && rel.has_parent_path()) {
            const std::string top = (*rel.begin()).generic_string();
            if (is_domain(top)) {
                file.folder_domain = top;
            }
        }
        std::ifstream in(path);
        if (!in) {
            result.rejections.push_back(file.path + ": cannot open file");
            continue;
        }
        static const std::set<std::string> known = {
            "function", "domain",       "description", "inputs",
            "outputs",  "capability",   "timeout_seconds",
            "depends_on",
        };
        std::string line;
        bool bad_line = false;
        while (std::getline(in, line)) {
            std::string t = trim(line);
            if (t.empty() || t[0] != '#') {
                continue;
            }
            t = trim(t.substr(1));
            static const std::string tag = "@jocky:";
            if (t.compare(0, tag.size(), tag) != 0) {
                continue;
            }
            file.has_any_header = true;
            std::string rest = trim(t.substr(tag.size()));
            const std::size_t sp = rest.find_first_of(" \t");
            const std::string key =
                (sp == std::string::npos) ? rest : rest.substr(0, sp);
            const std::string value =
                (sp == std::string::npos) ? "" : trim(rest.substr(sp));
            if (known.count(key) == 0) {
                result.rejections.push_back(
                    file.path + ": unknown @jocky: key '" + key + "'");
                bad_line = true;
                break;
            }
            if (file.keys.count(key) != 0) {
                result.rejections.push_back(
                    file.path + ": duplicate @jocky: key '" + key + "'");
                bad_line = true;
                break;
            }
            file.keys[key] = value;
        }
        if (bad_line) {
            continue;
        }
        if (!file.has_any_header) {
            result.skipped.push_back(
                file.path + ": no @jocky: header (not a registry function)");
            continue;
        }
        parsed.push_back(std::move(file));
    }

    // Field-level validation; structurally valid entries go to candidates.
    struct Candidate {
        ScriptMetadata meta;
    };
    std::vector<Candidate> candidates;
    std::set<std::string> seen_functions;
    for (const ParsedFile& file : parsed) {
        auto need = [&](const std::string& key, std::string& out) -> bool {
            auto found = file.keys.find(key);
            if (found == file.keys.end() || trim(found->second).empty()) {
                result.rejections.push_back(file.path + ": missing required " +
                                            std::string("@jocky:") + key);
                return false;
            }
            out = trim(found->second);
            return true;
        };
        std::string function, domain, description, inputs_raw, outputs_raw,
            capability;
        if (!need("function", function) || !need("domain", domain) ||
            !need("description", description) || !need("inputs", inputs_raw) ||
            !need("outputs", outputs_raw) || !need("capability", capability)) {
            continue;
        }
        if (!is_domain(domain)) {
            result.rejections.push_back(file.path + ": unknown domain '" +
                                        domain + "'");
            continue;
        }
        std::string name_domain;
        if (!check_function_name(function, name_domain)) {
            result.rejections.push_back(
                file.path + ": function name '" + function +
                "' does not match jky_<domain>_<verb>_<object>");
            continue;
        }
        if (name_domain != domain) {
            result.rejections.push_back(
                file.path + ": function domain segment '" + name_domain +
                "' disagrees with @jocky:domain '" + domain + "'");
            continue;
        }
        if (!file.folder_domain.empty() && file.folder_domain != domain) {
            result.rejections.push_back(
                file.path + ": @jocky:domain '" + domain +
                "' mismatches containing folder '" + file.folder_domain +
                "' (shared/ and scan-root files are exempt)");
            continue;
        }
        if (seen_functions.count(function) != 0) {
            result.rejections.push_back(file.path + ": duplicate function " +
                                        "name '" + function + "'");
            continue;
        }
        seen_functions.insert(function);

        ScriptMetadata meta;
        meta.function = function;
        meta.domain = domain;
        meta.description = description;
        meta.capability = capability;
        meta.script_path = file.path;
        try {
            meta.source_sha256 = sha256_file(file.path);
        } catch (const std::exception& ex) {
            result.rejections.push_back(file.path + ": " + ex.what());
            continue;
        }


        bool inputs_ok = true;
        for (const std::string& item : split_list(inputs_raw)) {
            const std::size_t colon = item.find(':');
            if (colon == std::string::npos) {
                result.rejections.push_back(
                    file.path + ": malformed input entry '" + item +
                    "' (expected name: type)");
                inputs_ok = false;
                break;
            }
            std::string name = trim(item.substr(0, colon));
            std::string type = trim(item.substr(colon + 1));
            // Phase 3 `= default` decision: split off and STORE the
            // default instead of dropping it (see script_metadata.hpp).
            InputParam param;
            const std::size_t eq = type.find('=');
            if (eq != std::string::npos) {
                std::string raw_default = trim(type.substr(eq + 1));
                type = trim(type.substr(0, eq));
                if (raw_default.empty()) {
                    result.rejections.push_back(
                        file.path + ": empty default for input '" + name +
                        "' (write `name: type` with no `=` when there is " +
                        "no default)");
                    inputs_ok = false;
                    break;
                }
                if (raw_default.size() >= 2 &&
                    raw_default.front() == '"' && raw_default.back() == '"') {
                    raw_default =
                        raw_default.substr(1, raw_default.size() - 2);
                }
                param.has_default = true;
                param.default_value = raw_default;
            }
            if (name.empty() || type.empty()) {
                result.rejections.push_back(
                    file.path + ": malformed input entry '" + item + "'");
                inputs_ok = false;
                break;
            }
            param.name = name;
            param.type = type;
            // The default must itself satisfy the declared type, else the
            // header promises a value the script may not accept.
            if (param.has_default &&
                !default_matches_type(param.default_value, param.type)) {
                result.rejections.push_back(
                    file.path + ": default '" + param.default_value +
                    "' for input '" + name + "' does not match declared " +
                    "type '" + type + "'");
                inputs_ok = false;
                break;
            }
            meta.inputs.push_back(std::move(param));
        }
        if (!inputs_ok) {
            continue;
        }
        {
            const std::size_t colon = outputs_raw.find(':');
            meta.output_type = (colon == std::string::npos)
                                   ? trim(outputs_raw)
                                   : trim(outputs_raw.substr(colon + 1));
            if (meta.output_type.empty()) {
                result.rejections.push_back(file.path +
                                            ": empty @jocky:outputs type");
                continue;
            }
        }
        auto timeout_it = file.keys.find("timeout_seconds");
        if (timeout_it != file.keys.end() && !trim(timeout_it->second).empty()) {
            try {
                std::size_t used = 0;
                int value = std::stoi(trim(timeout_it->second), &used);
                if (used != trim(timeout_it->second).size() || value <= 0) {
                    throw std::invalid_argument("range");
                }
                meta.timeout_seconds = value;
            } catch (const std::exception&) {
                result.rejections.push_back(
                    file.path + ": bad @jocky:timeout_seconds '" +
                    trim(timeout_it->second) + "' (want positive int)");
                continue;
            }
        }
        auto deps_it = file.keys.find("depends_on");
        if (deps_it != file.keys.end()) {
            std::string flat = trim(deps_it->second);
            for (char& c : flat) {
                if (c == ',') {
                    c = ' ';
                }
            }
            meta.depends_on = split_words(flat);
        }
        candidates.push_back(Candidate{std::move(meta)});
    }

    // Dependency resolution against the structurally valid set.
    std::set<std::string> available;
    for (const Candidate& cand : candidates) {
        available.insert(cand.meta.function);
    }
    for (const Candidate& cand : candidates) {
        bool ok = true;
        for (const std::string& dep : cand.meta.depends_on) {
            if (available.count(dep) == 0) {
                result.rejections.push_back(
                    cand.meta.script_path + ": unresolvable depends_on '" +
                    dep + "'");
                ok = false;
            }
        }
        if (ok) {
            result.registry.push_back(cand.meta);
            result.per_domain_counts[cand.meta.domain] += 1;
        }
    }
    std::sort(result.registry.begin(), result.registry.end(),
              [](const ScriptMetadata& a, const ScriptMetadata& b) {
                  return a.function < b.function;
              });
    return result;
}

std::string registry_to_json(const ScanResult& result) {
    std::string out = "{\"functions\":[";
    bool first_fn = true;
    for (const ScriptMetadata& meta : result.registry) {
        if (!first_fn) {
            out += ",";
        }
        first_fn = false;
        out += "{\"function\":\"" + json_escape(meta.function) + "\"";
        out += ",\"domain\":\"" + json_escape(meta.domain) + "\"";
        out += ",\"description\":\"" + json_escape(meta.description) + "\"";
        out += ",\"inputs\":[";
        bool first_in = true;
        for (const auto& input : meta.inputs) {
            if (!first_in) {
                out += ",";
            }
            first_in = false;
            out += "{\"name\":\"" + json_escape(input.name) +
                   "\",\"type\":\"" + json_escape(input.type) + "\"" +
                   ",\"has_default\":" +
                   (input.has_default ? "true" : "false") +
                   ",\"default\":\"" + json_escape(input.default_value) +
                   "\"}";
        }
        out += "]";
        out += ",\"output_type\":\"" + json_escape(meta.output_type) + "\"";
        out += ",\"capability\":\"" + json_escape(meta.capability) + "\"";
        out += ",\"timeout_seconds\":" + std::to_string(meta.timeout_seconds);
        out += ",\"depends_on\":[";
        bool first_dep = true;
        for (const std::string& dep : meta.depends_on) {
            if (!first_dep) {
                out += ",";
            }
            first_dep = false;
            out += "\"" + json_escape(dep) + "\"";
        }
        out += "]";
        out += ",\"source_sha256\":\"" +
               json_escape(meta.source_sha256) + "\"";
        out += ",\"script_path\":\"" + json_escape(meta.script_path) + "\"}";
    }
    out += "]}";
    return out;
}

}  // namespace jocky
