#pragma once
// JOCKY manifest verifier — re-hashes artifacts recorded in a manifest
// and re-checks per-execution integrity entries. Reads only the manifest
// file and the artifacts it names; never executes scripts, never touches
// the registry, never opens a socket.
//
// What is checked, in order:
//   1. Manifest parses as schema 0.2.0 JSON (manifest_version field).
//   2. Every inputs[] / outputs[] artifact re-hashes to its recorded
//      sha256 (files via sha256_file; directories via the same sorted
//      inventory hash the dispatcher uses).
//   3. Every executions[] entry with a non-empty script_sha256 and a
//      recorded stdout re-hashes stdout to stdout_sha256.
//   4. Structural sanity: executions[] present, status is a known value,
//      no entry stuck at the "started" placeholder outcome.
//
// Anything else (timestamps, durations, exit codes, stderr text) is
// reported data, not integrity evidence, and is not re-checked.
// Reports PASS (exit 0) or FAIL naming every mismatch (exit 1).

#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "jocky/crypto/sha256.hpp"
#include "jocky/runtime/dispatcher.hpp"

namespace jocky {

struct VerifyIssue {
    std::string where;
    std::string message;
};

struct VerifyResult {
    bool pass = false;
    std::vector<VerifyIssue> issues;
};

namespace verify_detail {

// Minimal JSON reader sufficient for manifests the dispatcher writes:
// objects with string keys, string/number/bool values, nested
// objects/arrays. Not a general parser; rejects anything outside the
// manifest shape with a plain error instead of guessing.
struct JsonValue {
    enum class Kind { Str, Num, Bool, Obj, Arr };
    Kind kind = Kind::Str;
    std::string str;
    std::vector<std::pair<std::string, JsonValue>> obj;
    std::vector<JsonValue> arr;
};

struct JsonReader {
    std::string text;
    std::size_t pos = 0;

    explicit JsonReader(std::string text) : text(std::move(text)) {}

    [[noreturn]] void fail(const std::string& what) {
        throw std::runtime_error("manifest is not valid JSON (" + what +
                                 " near byte " + std::to_string(pos) + ")");
    }

    void skip_ws() {
        while (pos < text.size() &&
               (text[pos] == ' ' || text[pos] == '\t' ||
                text[pos] == '\n' || text[pos] == '\r')) {
            ++pos;
        }
    }

    char take() {
        if (pos >= text.size()) fail("unexpected end of input");
        return text[pos++];
    }

    std::string parse_string() {
        if (take() != '"') fail("expected string");
        std::string out;
        while (true) {
            if (pos >= text.size()) fail("unterminated string");
            const char c = text[pos++];
            if (c == '"') return out;
            if (c != '\\') {
                out += c;
                continue;
            }
            if (pos >= text.size()) fail("bad escape");
            const char esc = text[pos++];
            switch (esc) {
                case '"': out += '"'; break;
                case '\\': out += '\\'; break;
                case '/': out += '/'; break;
                case 'b': out += '\b'; break;
                case 'f': out += '\f'; break;
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case 't': out += '\t'; break;
                case 'u': {
                    if (pos + 4 > text.size()) fail("bad unicode escape");
                    unsigned code = 0;
                    for (int i = 0; i < 4; ++i) {
                        const char h = text[pos++];
                        code <<= 4;
                        if (h >= '0' && h <= '9') code += h - '0';
                        else if (h >= 'a' && h <= 'f') code += h - 'a' + 10;
                        else if (h >= 'A' && h <= 'F') code += h - 'A' + 10;
                        else fail("bad unicode escape");
                    }
                    if (code < 0x80) {
                        out += static_cast<char>(code);
                    } else if (code < 0x800) {
                        out += static_cast<char>(0xc0 | (code >> 6));
                        out += static_cast<char>(0x80 | (code & 0x3f));
                    } else {
                        out += static_cast<char>(0xe0 | (code >> 12));
                        out += static_cast<char>(0x80 | ((code >> 6) & 0x3f));
                        out += static_cast<char>(0x80 | (code & 0x3f));
                    }
                    break;
                }
                default: fail("bad escape");
            }
        }
    }

    JsonValue parse_value() {
        skip_ws();
        if (pos >= text.size()) fail("unexpected end of input");
        const char c = text[pos];
        if (c == '"') {
            JsonValue value;
            value.kind = JsonValue::Kind::Str;
            value.str = parse_string();
            return value;
        }
        if (c == '{') {
            ++pos;
            JsonValue value;
            value.kind = JsonValue::Kind::Obj;
            skip_ws();
            if (pos < text.size() && text[pos] == '}') {
                ++pos;
                return value;
            }
            while (true) {
                skip_ws();
                std::string key = parse_string();
                skip_ws();
                if (take() != ':') fail("expected ':'");
                value.obj.emplace_back(key, parse_value());
                skip_ws();
                const char sep = take();
                if (sep == '}') return value;
                if (sep != ',') fail("expected ',' or '}'");
            }
        }
        if (c == '[') {
            ++pos;
            JsonValue value;
            value.kind = JsonValue::Kind::Arr;
            skip_ws();
            if (pos < text.size() && text[pos] == ']') {
                ++pos;
                return value;
            }
            while (true) {
                value.arr.push_back(parse_value());
                skip_ws();
                const char sep = take();
                if (sep == ']') return value;
                if (sep != ',') fail("expected ',' or ']'");
            }
        }
        if (c == 't' && text.compare(pos, 4, "true") == 0) {
            pos += 4;
            JsonValue value;
            value.kind = JsonValue::Kind::Bool;
            value.str = "true";
            return value;
        }
        if (c == 'f' && text.compare(pos, 5, "false") == 0) {
            pos += 5;
            JsonValue value;
            value.kind = JsonValue::Kind::Bool;
            value.str = "false";
            return value;
        }
        if (c == 'n' && text.compare(pos, 4, "null") == 0) {
            pos += 4;
            JsonValue value;
            value.kind = JsonValue::Kind::Str;
            value.str = "";
            return value;
        }
        if (c == '-' || (c >= '0' && c <= '9')) {
            const std::size_t start = pos;
            if (c == '-') ++pos;
            while (pos < text.size() &&
                   (std::isdigit(static_cast<unsigned char>(text[pos])) ||
                    text[pos] == '.' || text[pos] == 'e' ||
                    text[pos] == 'E' || text[pos] == '+' ||
                    text[pos] == '-')) {
                ++pos;
            }
            JsonValue value;
            value.kind = JsonValue::Kind::Num;
            value.str = text.substr(start, pos - start);
            return value;
        }
        fail("unexpected value");
    }

    const JsonValue* find(const JsonValue& obj, const std::string& key) {
        if (obj.kind != JsonValue::Kind::Obj) return nullptr;
        for (const auto& [name, value] : obj.obj) {
            if (name == key) return &value;
        }
        return nullptr;
    }

    std::string need_str(const JsonValue& obj, const std::string& key,
                         const std::string& where,
                         std::vector<VerifyIssue>& issues) {
        const JsonValue* found = find(obj, key);
        if (found == nullptr || found->kind != JsonValue::Kind::Str) {
            issues.push_back(
                {where, "missing or non-string field '" + key + "'"});
            return "";
        }
        return found->str;
    }
};

}  // namespace verify_detail

// Verify a manifest file on disk. All artifact paths in the manifest are
// resolved relative to `base_dir` (the manifest's own directory) unless
// absolute. Returns pass/fail with one issue per mismatch.
inline VerifyResult verify_manifest(const std::string& manifest_path) {
    namespace detail = verify_detail;
    VerifyResult result;
    std::ifstream in(manifest_path, std::ios::binary);
    if (!in) {
        result.issues.push_back(
            {"manifest", "cannot open file '" + manifest_path + "'"});
        return result;
    }
    std::ostringstream text;
    text << in.rdbuf();
    detail::JsonReader reader(text.str());
    detail::JsonValue root;
    try {
        root = reader.parse_value();
    } catch (const std::exception& ex) {
        result.issues.push_back({"manifest", ex.what()});
        return result;
    }
    if (root.kind != detail::JsonValue::Kind::Obj) {
        result.issues.push_back({"manifest", "top-level value is not an object"});
        return result;
    }
    const std::string version =
        detail::JsonReader(text.str())
            .need_str(root, "manifest_version", "manifest", result.issues);
    if (version != "0.2.0") {
        result.issues.push_back(
            {"manifest",
             "unsupported manifest_version '" + version + "' (need 0.2.0)"});
    }
    const detail::JsonValue* status = reader.find(root, "status");
    if (status == nullptr || status->kind != detail::JsonValue::Kind::Str ||
        (status->str != "success" && status->str != "failed" &&
         status->str != "preflight_failed")) {
        result.issues.push_back({"manifest", "unknown status value"});
    }
    const std::string base =
        std::filesystem::path(manifest_path).parent_path().string();
    auto resolve = [&](const std::string& path) {
        if (path.empty()) return path;
        if (std::filesystem::path(path).is_absolute()) return path;
        if (base.empty()) return path;
        return (std::filesystem::path(base) / path).string();
    };
    auto check_artifact =
        [&](const detail::JsonValue& item, const std::string& where) {
            if (item.kind != detail::JsonValue::Kind::Obj) {
                result.issues.push_back({where, "artifact is not an object"});
                return;
            }
            std::string name =
                reader.need_str(item, "name", where, result.issues);
            std::string path =
                reader.need_str(item, "path", where, result.issues);
            std::string want =
                reader.need_str(item, "sha256", where, result.issues);
            if (path.empty() || want.empty()) return;
            const std::string full = resolve(path);
            std::string got;
            try {
                got = runtime_detail::hash_artifact(name, "", full).sha256;
            } catch (const std::exception& ex) {
                result.issues.push_back({where, ex.what()});
                return;
            }
            if (got != want) {
                result.issues.push_back(
                    {where,
                     "artifact '" + path + "' hash mismatch: manifest " +
                         want + ", recomputed " + got});
            }
        };
    const detail::JsonValue* inputs = reader.find(root, "inputs");
    if (inputs != nullptr) {
        if (inputs->kind != detail::JsonValue::Kind::Arr) {
            result.issues.push_back({"inputs", "not an array"});
        } else {
            for (std::size_t i = 0; i < inputs->arr.size(); ++i) {
                check_artifact(inputs->arr[i],
                               "inputs[" + std::to_string(i) + "]");
            }
        }
    }
    const detail::JsonValue* outputs = reader.find(root, "outputs");
    if (outputs != nullptr) {
        if (outputs->kind != detail::JsonValue::Kind::Arr) {
            result.issues.push_back({"outputs", "not an array"});
        } else {
            for (std::size_t i = 0; i < outputs->arr.size(); ++i) {
                check_artifact(outputs->arr[i],
                               "outputs[" + std::to_string(i) + "]");
            }
        }
    }
    const detail::JsonValue* executions = reader.find(root, "executions");
    if (executions == nullptr ||
        executions->kind != detail::JsonValue::Kind::Arr) {
        result.issues.push_back({"executions", "missing or not an array"});
    } else {
        for (std::size_t i = 0; i < executions->arr.size(); ++i) {
            const std::string where =
                "executions[" + std::to_string(i) + "]";
            const detail::JsonValue& entry = executions->arr[i];
            if (entry.kind != detail::JsonValue::Kind::Obj) {
                result.issues.push_back({where, "entry is not an object"});
                continue;
            }
            std::string outcome =
                reader.need_str(entry, "outcome", where, result.issues);
            if (outcome == "started") {
                result.issues.push_back(
                    {where, "entry stuck at placeholder outcome 'started'"});
            }
            const detail::JsonValue* recorded =
                reader.find(entry, "script_sha256");
            const detail::JsonValue* stdout_text =
                reader.find(entry, "stdout");
            const detail::JsonValue* stdout_hash =
                reader.find(entry, "stdout_sha256");
            if (recorded != nullptr &&
                recorded->kind == detail::JsonValue::Kind::Str &&
                !recorded->str.empty() && stdout_text != nullptr &&
                stdout_text->kind == detail::JsonValue::Kind::Str &&
                stdout_hash != nullptr &&
                stdout_hash->kind == detail::JsonValue::Kind::Str &&
                !stdout_hash->str.empty()) {
                const std::string recomputed =
                    sha256_bytes(stdout_text->str);
                if (recomputed != stdout_hash->str) {
                    result.issues.push_back(
                        {where,
                         "stdout hash mismatch: manifest " +
                             stdout_hash->str + ", recomputed " +
                             recomputed});
                }
            }
        }
    }
    result.pass = result.issues.empty();
    return result;
}

}  // namespace jocky
