#pragma once
// Phase 7 shared runtime dispatcher.
//
// Security properties:
// - a manifest exists before any child is spawned and is rewritten after every
//   completed attempt;
// - capabilities and concrete argument types are rechecked at dispatch;
// - scripts receive positional argv entries, never a shell command string;
// - a per-call chroot contains copied input snapshots and staged outputs;
//   user, network, and PID namespaces keep host evidence paths unreachable;
// - every child is placed in a process group and bounded by a wall timeout.

#include <algorithm>
#include <chrono>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <poll.h>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "jocky/crypto/sha256.hpp"
#include "jocky/stdlib/script_metadata.hpp"

namespace jocky {

struct RuntimeArgument {
    std::string name;
    std::string declared_type;
    std::string value;
    bool concrete = true;
};

struct RuntimeCall {
    std::string function;
    std::string capability;
    std::string script_sha256;
    std::string script_source;
    int timeout_seconds = 30;
    std::vector<RuntimeArgument> args;
    // Source position of the `call` keyword (Phase 7.5). The
    // control-flow executor matches re-parsed AST call sites to plan
    // entries through (line, col); 0/0 means "unpositioned" (hand-built
    // plans, which only run in flat legacy mode).
    int line = 0;
    int col = 0;
};

inline RuntimeCall load_registry_runtime_call(
    const ScriptMetadata& metadata, std::vector<RuntimeArgument> args) {
    RuntimeCall call;
    call.function = metadata.function;
    call.capability = metadata.capability;
    call.script_sha256 = metadata.source_sha256;
    call.timeout_seconds = metadata.timeout_seconds;
    call.args = std::move(args);
    call.script_source = read_binary_file(metadata.script_path);
    // Preserve the scanner digest even when bytes drift. The shared
    // dispatcher performs the terminal recheck after creating the manifest,
    // so a stale registry attempt becomes a logged integrity_denied entry.
    return call;
}

struct RuntimeEvidence {
    std::string name;
    std::string adapter;
    std::string path;
};

struct RuntimePlan {
    std::string case_id;
    // SHA-256 of the .jky program text (provenance, not a script hash;
    // serialized as top-level "program_sha256" since Phase 7.5 — the old
    // "script_sha256" key name was a wart, see wiki/18-phase6-7-audit.md
    // GAP-5).
    std::string program_sha256;
    std::vector<std::string> allowed_capabilities;
    std::vector<RuntimeEvidence> evidence;
    std::vector<std::string> declared_outputs;
    std::vector<RuntimeCall> calls;
    // Embedded .jky source for execution-time control flow (Phase 7.5).
    // The compiled binary always sets this; the executor re-parses it
    // and walks the real statement tree. Empty means "no tree": legacy
    // flat dispatch of `calls` in order (hand-built plans, all Phase 7
    // tests). Never part of the manifest.
    std::string program_source;
    // Per-case while-loop iteration ceiling from
    // `max_while_iterations` (Phase 7.5). 0 means "unset": fall back to
    // RuntimeOptions::max_while_iterations.
    std::size_t max_while_iterations = 0;
};

struct RuntimeOptions {
    std::string executable_path;
    std::string working_directory;
    std::string output_root = "out";
    std::string manifest_path = "out/manifest.json";
    std::size_t max_executions = 1000;
    // Default while-loop iteration ceiling (Phase 7.5, fulfills the
    // wiki/17 §3 commitment). A case-level `max_while_iterations`
    // overrides this per plan. Hitting it aborts the loop with a
    // distinct "while_ceiling" manifest entry — never silent.
    std::size_t max_while_iterations = 10000;
};

struct ExecutionRecord {
    std::string function;
    std::string capability;
    std::string script_sha256;
    std::vector<RuntimeArgument> args;
    std::string outcome = "started";
    int exit_code = -1;
    bool timed_out = false;
    long long duration_ms = 0;
    // Phase 7.5 chain-of-custody timing: wall-clock start/end of this
    // attempt (UTC, second resolution like the run stamps).
    // duration_ms stays as the convenient millisecond measure.
    std::string start_utc;
    std::string end_utc;
    std::string stdout_text;
    std::string stderr_text;
    // Phase 7.5: SHA-256 of the captured stdout bytes, recorded
    // alongside the raw text (kept for debugging). Set when the entry
    // reaches a terminal outcome.
    std::string stdout_sha256;
};

struct ArtifactRecord {
    std::string name;
    std::string adapter;
    std::string path;
    std::string sha256;
    std::uintmax_t bytes = 0;
};

struct RuntimeManifest {
    std::string case_id;
    std::string program_sha256;
    std::string run_start_utc;
    std::string run_end_utc;
    std::string status = "running";
    std::vector<std::string> allowed_capabilities;
    std::vector<ArtifactRecord> inputs;
    std::vector<ArtifactRecord> outputs;
    std::vector<ExecutionRecord> executions;
    std::vector<std::string> errors;
};

struct DispatchResult {
    int exit_code = 1;
    RuntimeManifest manifest;
};

namespace runtime_detail {

namespace fs = std::filesystem;

inline std::string json_escape(const std::string& text) {
    std::string out;
    for (unsigned char c : text) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20U) {
                    const char hex[] = "0123456789abcdef";
                    out += "\\u00";
                    out += hex[c >> 4U];
                    out += hex[c & 0x0fU];
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    return out;
}

inline std::string utc_now() {
    const std::time_t now = std::time(nullptr);
    std::tm value{};
    gmtime_r(&now, &value);
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &value);
    return buffer;
}

inline fs::path canonical_path(const fs::path& input,
                               const fs::path& working_directory) {
    const fs::path absolute =
        input.is_absolute() ? input : (working_directory / input);
    std::error_code ec;
    fs::path result = fs::weakly_canonical(absolute, ec);
    if (ec) {
        throw std::runtime_error("cannot canonicalize path '" +
                                 absolute.string() + "': " + ec.message());
    }
    return result.lexically_normal();
}

inline bool path_is_within(const fs::path& child, const fs::path& root) {
    auto child_it = child.begin();
    auto root_it = root.begin();
    for (; root_it != root.end(); ++root_it, ++child_it) {
        if (child_it == child.end() || *child_it != *root_it) {
            return false;
        }
    }
    return true;
}

inline ArtifactRecord hash_artifact(const std::string& name,
                                    const std::string& adapter,
                                    const fs::path& path) {
    std::error_code ec;
    const fs::file_status status = fs::symlink_status(path, ec);
    if (ec || !fs::exists(status)) {
        throw std::runtime_error("artifact does not exist: '" +
                                 path.string() + "'");
    }
    if (fs::is_symlink(status)) {
        throw std::runtime_error("symbolic-link artifact refused: '" +
                                 path.string() + "'");
    }

    ArtifactRecord record;
    record.name = name;
    record.adapter = adapter;
    record.path = path.string();
    if (fs::is_regular_file(status)) {
        record.bytes = fs::file_size(path);
        record.sha256 = sha256_file(path.string());
        return record;
    }
    if (!fs::is_directory(status)) {
        throw std::runtime_error("unsupported artifact type: '" +
                                 path.string() + "'");
    }

    std::vector<fs::path> files;
    for (fs::recursive_directory_iterator it(path), end; it != end; ++it) {
        const fs::file_status child_status = it->symlink_status();
        if (fs::is_symlink(child_status)) {
            throw std::runtime_error(
                "symbolic link inside directory evidence refused: '" +
                it->path().string() + "'");
        }
        if (fs::is_regular_file(child_status)) {
            files.push_back(it->path());
        }
    }
    std::sort(files.begin(), files.end());
    std::string inventory;
    for (const fs::path& file : files) {
        const fs::path relative = fs::relative(file, path);
        const std::uintmax_t bytes = fs::file_size(file);
        inventory += relative.generic_string();
        inventory.push_back('\0');
        inventory += sha256_file(file.string());
        inventory.push_back('\0');
        inventory += std::to_string(bytes);
        inventory.push_back('\n');
        record.bytes += bytes;
    }
    record.sha256 = sha256_bytes(inventory);
    return record;
}

inline void write_json_string(std::ostream& out, const std::string& value) {
    out << "\"" << json_escape(value) << "\"";
}

inline void write_artifacts(std::ostream& out,
                            const std::vector<ArtifactRecord>& artifacts) {
    out << "[";
    for (std::size_t i = 0; i < artifacts.size(); ++i) {
        if (i != 0) out << ",";
        const ArtifactRecord& item = artifacts[i];
        out << "{\"name\":";
        write_json_string(out, item.name);
        out << ",\"adapter\":";
        write_json_string(out, item.adapter);
        out << ",\"path\":";
        write_json_string(out, item.path);
        out << ",\"sha256\":";
        write_json_string(out, item.sha256);
        out << ",\"bytes\":" << item.bytes << "}";
    }
    out << "]";
}

inline void write_manifest_json(std::ostream& out,
                                const RuntimeManifest& manifest) {
    // Schema 0.2.0 (Phase 7.5): top-level "program_sha256" renames the
    // Phase 7 "script_sha256" key, which misleadingly held the .jky
    // source hash (audit GAP-5); per-execution entries gain start_utc /
    // end_utc / stdout_sha256. Raw stdout/stderr and duration_ms stay.
    out << "{\"manifest_version\":\"0.2.0\",\"case_id\":";
    write_json_string(out, manifest.case_id);
    out << ",\"program_sha256\":";
    write_json_string(out, manifest.program_sha256);
    out << ",\"runtime_version\":\"jocky 0.1.0\",\"run_start_utc\":";
    write_json_string(out, manifest.run_start_utc);
    out << ",\"run_end_utc\":";
    write_json_string(out, manifest.run_end_utc);
    out << ",\"status\":";
    write_json_string(out, manifest.status);
    out << ",\"authorization\":{\"case\":";
    write_json_string(out, manifest.case_id);
    out << ",\"allowed_capabilities\":[";
    for (std::size_t i = 0; i < manifest.allowed_capabilities.size(); ++i) {
        if (i != 0) out << ",";
        write_json_string(out, manifest.allowed_capabilities[i]);
    }
    out << "]},\"inputs\":";
    write_artifacts(out, manifest.inputs);
    out << ",\"outputs\":";
    write_artifacts(out, manifest.outputs);
    out << ",\"executions\":[";
    for (std::size_t i = 0; i < manifest.executions.size(); ++i) {
        if (i != 0) out << ",";
        const ExecutionRecord& item = manifest.executions[i];
        out << "{\"function\":";
        write_json_string(out, item.function);
        out << ",\"capability\":";
        write_json_string(out, item.capability);
        out << ",\"script_sha256\":";
        write_json_string(out, item.script_sha256);
        out << ",\"args\":{";
        for (std::size_t j = 0; j < item.args.size(); ++j) {
            if (j != 0) out << ",";
            write_json_string(out, item.args[j].name);
            out << ":";
            write_json_string(out, item.args[j].value);
        }
        out << "},\"outcome\":";
        write_json_string(out, item.outcome);
        out << ",\"exit_code\":" << item.exit_code;
        out << ",\"timed_out\":" << (item.timed_out ? "true" : "false");
        out << ",\"duration_ms\":" << item.duration_ms;
        out << ",\"start_utc\":";
        write_json_string(out, item.start_utc);
        out << ",\"end_utc\":";
        write_json_string(out, item.end_utc);
        out << ",\"stdout\":";
        write_json_string(out, item.stdout_text);
        out << ",\"stdout_sha256\":";
        write_json_string(out, item.stdout_sha256);
        out << ",\"stderr\":";
        write_json_string(out, item.stderr_text);
        out << "}";
    }
    out << "],\"errors\":[";
    for (std::size_t i = 0; i < manifest.errors.size(); ++i) {
        if (i != 0) out << ",";
        write_json_string(out, manifest.errors[i]);
    }
    out << "]}\n";
}

inline void persist_manifest(const fs::path& manifest_path,
                             const RuntimeManifest& manifest) {
    const fs::path temp =
        manifest_path.string() + ".tmp." + std::to_string(getpid());
    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        if (!out) {
            throw std::runtime_error("cannot create manifest temp file '" +
                                     temp.string() + "'");
        }
        write_manifest_json(out, manifest);
        out.flush();
        if (!out) {
            throw std::runtime_error("cannot write manifest temp file '" +
                                     temp.string() + "'");
        }
    }
    std::error_code ec;
    fs::rename(temp, manifest_path, ec);
    if (ec) {
        fs::remove(temp);
        throw std::runtime_error("cannot publish manifest '" +
                                 manifest_path.string() + "': " +
                                 ec.message());
    }
}

inline bool capability_allowed(const std::vector<std::string>& allowed,
                               const std::string& required) {
    return std::find(allowed.begin(), allowed.end(), required) != allowed.end();
}

inline bool valid_runtime_value(const RuntimeArgument& arg) {
    if (!arg.concrete) return false;
    if (arg.declared_type == "string" || arg.declared_type == "path") {
        return true;
    }
    if (arg.declared_type == "bool") {
        return arg.value == "true" || arg.value == "false";
    }
    if (arg.declared_type == "int") {
        long long parsed = 0;
        const char* begin = arg.value.data();
        const char* end = begin + arg.value.size();
        const auto result = std::from_chars(begin, end, parsed);
        return result.ec == std::errc() && result.ptr == end;
    }
    if (arg.declared_type == "float") {
        if (arg.value.empty()) return false;
        char* end = nullptr;
        errno = 0;
        const double parsed = std::strtod(arg.value.c_str(), &end);
        return errno == 0 && end == arg.value.c_str() + arg.value.size() &&
               std::isfinite(parsed);
    }
    return false;
}

inline std::string self_executable() {
    std::vector<char> buffer(4096);
    const ssize_t used =
        readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
    if (used <= 0) {
        throw std::runtime_error("cannot resolve /proc/self/exe");
    }
    return std::string(buffer.data(), static_cast<std::size_t>(used));
}

inline int sandbox_error(const std::string& message) {
    std::fprintf(stderr, "sandbox: %s: %s\n", message.c_str(),
                 std::strerror(errno));
    return 125;
}

inline int sandbox_child_main(int argc, char** argv) {
    // executable --jocky-sandbox-child rootfs args...
    if (argc < 3) {
        std::fprintf(stderr, "sandbox: incomplete internal arguments\n");
        return 125;
    }
    const fs::path rootfs = argv[2];
    if (chroot(rootfs.c_str()) != 0) {
        return sandbox_error("cannot enter isolated root");
    }
    if (chdir("/work") != 0) {
        return sandbox_error("cannot enter isolated working directory");
    }
    clearenv();
    setenv("PATH", "/usr/bin:/bin", 1);
    setenv("LC_ALL", "C", 1);
    setenv("HOME", "/nonexistent", 1);
    setenv("TMPDIR", "/tmp", 1);

    std::vector<char*> child_args;
    child_args.push_back(const_cast<char*>("/bin/bash"));
    child_args.push_back(const_cast<char*>("/script.sh"));
    for (int i = 3; i < argc; ++i) child_args.push_back(argv[i]);
    child_args.push_back(nullptr);
    execv(child_args[0], child_args.data());
    return sandbox_error("cannot execute isolated /bin/bash");
}

inline void copy_runtime_file(const fs::path& source, const fs::path& target) {
    std::error_code ec;
    fs::create_directories(target.parent_path(), ec);
    if (ec) {
        throw std::runtime_error("cannot create isolated runtime directory: " +
                                 ec.message());
    }
    fs::copy_file(source, target, fs::copy_options::overwrite_existing, ec);
    if (ec) {
        throw std::runtime_error("cannot copy isolated runtime file '" +
                                 source.string() + "': " + ec.message());
    }
}

inline void prepare_isolated_root(const fs::path& rootfs,
                                  const std::string& script_source) {
    fs::create_directories(rootfs / "bin");
    fs::create_directories(rootfs / "usr/bin");
    fs::create_directories(rootfs / "lib/x86_64-linux-gnu");
    fs::create_directories(rootfs / "lib64");
    fs::create_directories(rootfs / "inputs");
    fs::create_directories(rootfs / "outputs");
    fs::create_directories(rootfs / "work");
    fs::create_directories(rootfs / "tmp");

    copy_runtime_file("/usr/bin/bash", rootfs / "bin/bash");
    copy_runtime_file("/usr/bin/sleep", rootfs / "usr/bin/sleep");
    copy_runtime_file("/lib/x86_64-linux-gnu/libtinfo.so.6",
                      rootfs / "lib/x86_64-linux-gnu/libtinfo.so.6");
    copy_runtime_file("/lib/x86_64-linux-gnu/libc.so.6",
                      rootfs / "lib/x86_64-linux-gnu/libc.so.6");
    copy_runtime_file("/lib64/ld-linux-x86-64.so.2",
                      rootfs / "lib64/ld-linux-x86-64.so.2");

    std::ofstream script(rootfs / "script.sh",
                         std::ios::binary | std::ios::trunc);
    if (!script) {
        throw std::runtime_error("cannot materialize isolated script");
    }
    script.write(script_source.data(),
                 static_cast<std::streamsize>(script_source.size()));
    script.close();
    fs::permissions(rootfs / "script.sh", fs::perms::owner_read,
                    fs::perm_options::replace);
}

inline void make_snapshot_read_only(const fs::path& path) {
    std::error_code ec;
    if (fs::is_directory(path, ec)) {
        for (fs::recursive_directory_iterator it(path), end; it != end; ++it) {
            const fs::perms perms = fs::is_directory(it->symlink_status())
                                        ? fs::perms::owner_read |
                                              fs::perms::owner_exec
                                        : fs::perms::owner_read;
            fs::permissions(it->path(), perms, fs::perm_options::replace, ec);
            if (ec) {
                throw std::runtime_error("cannot protect evidence snapshot");
            }
        }
        fs::permissions(path, fs::perms::owner_read | fs::perms::owner_exec,
                        fs::perm_options::replace, ec);
    } else {
        fs::permissions(path, fs::perms::owner_read,
                        fs::perm_options::replace, ec);
    }
    if (ec) throw std::runtime_error("cannot protect evidence snapshot");
}

inline fs::path snapshot_input(const fs::path& source,
                               const fs::path& rootfs,
                               std::size_t index) {
    const fs::path target = rootfs / "inputs" / std::to_string(index);
    std::error_code ec;
    const fs::file_status status = fs::symlink_status(source, ec);
    if (ec || fs::is_symlink(status)) {
        throw std::runtime_error("symbolic-link path input refused: '" +
                                 source.string() + "'");
    }
    if (fs::is_directory(status)) {
        fs::copy(source, target, fs::copy_options::recursive, ec);
    } else if (fs::is_regular_file(status)) {
        fs::copy_file(source, target, fs::copy_options::overwrite_existing, ec);
    } else {
        throw std::runtime_error("path input does not exist or is unsupported: '" +
                                 source.string() + "'");
    }
    if (ec) {
        throw std::runtime_error("cannot snapshot path input '" +
                                 source.string() + "': " + ec.message());
    }
    make_snapshot_read_only(target);
    return fs::path("/inputs") / std::to_string(index);
}

inline fs::path staged_output_path(const fs::path& output_root,
                                   const fs::path& destination) {
    return fs::path("/outputs") / fs::relative(destination, output_root);
}

inline void promote_output(const fs::path& rootfs,
                           const fs::path& staged,
                           const fs::path& destination) {
    const fs::path host_staged = rootfs / staged.relative_path();
    if (!fs::exists(host_staged)) return;
    const fs::file_status status = fs::symlink_status(host_staged);
    if (fs::is_symlink(status)) {
        throw std::runtime_error("symbolic-link output refused: '" +
                                 destination.string() + "'");
    }
    fs::create_directories(destination.parent_path());
    std::error_code ec;
    if (fs::is_directory(status)) {
        fs::remove_all(destination, ec);
        ec.clear();
        fs::copy(host_staged, destination, fs::copy_options::recursive, ec);
    } else if (fs::is_regular_file(status)) {
        const fs::path temporary =
            destination.string() + ".jocky.tmp." + std::to_string(getpid());
        fs::copy_file(host_staged, temporary,
                      fs::copy_options::overwrite_existing, ec);
        if (!ec) {
            fs::rename(temporary, destination, ec);
            if (ec) fs::remove(temporary);
        }
    } else {
        throw std::runtime_error("unsupported staged output type");
    }
    if (ec) {
        throw std::runtime_error("cannot promote declared output '" +
                                 destination.string() + "': " + ec.message());
    }
}

inline void drain_fd(int fd, std::string& output, bool& open) {
    char buffer[8192];
    while (open) {
        const ssize_t used = read(fd, buffer, sizeof(buffer));
        if (used > 0) {
            output.append(buffer, static_cast<std::size_t>(used));
            continue;
        }
        if (used == 0) {
            close(fd);
            open = false;
        } else if (errno != EAGAIN && errno != EWOULDBLOCK &&
                   errno != EINTR) {
            close(fd);
            open = false;
        }
        break;
    }
}

struct ChildResult {
    int exit_code = 1;
    bool timed_out = false;
    long long duration_ms = 0;
    std::string stdout_text;
    std::string stderr_text;
};

inline ChildResult spawn_sandboxed(const RuntimeOptions& options,
                                   const std::vector<std::string>& values,
                                   int timeout_seconds,
                                   const fs::path& sandbox_root) {
    int stdout_pipe[2];
    int stderr_pipe[2];
    if (pipe(stdout_pipe) != 0 || pipe(stderr_pipe) != 0) {
        throw std::runtime_error("cannot create capture pipes");
    }
    const auto started = std::chrono::steady_clock::now();
    const pid_t child = fork();
    if (child < 0) {
        throw std::runtime_error("cannot fork sandbox process");
    }
    if (child == 0) {
        setpgid(0, 0);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);
        close(stdout_pipe[0]); close(stdout_pipe[1]);
        close(stderr_pipe[0]); close(stderr_pipe[1]);

        std::vector<std::string> owned = {
            "unshare", "--user", "--map-root-user", "--net", "--pid",
            "--fork", "--kill-child=KILL", options.executable_path,
            "--jocky-sandbox-child", sandbox_root.string()};
        owned.insert(owned.end(), values.begin(), values.end());
        std::vector<char*> args;
        for (std::string& item : owned) args.push_back(item.data());
        args.push_back(nullptr);
        execvp(args[0], args.data());
        std::fprintf(stderr, "sandbox: cannot execute unshare: %s\n",
                     std::strerror(errno));
        _exit(125);
    }
    setpgid(child, child);
    close(stdout_pipe[1]);
    close(stderr_pipe[1]);
    fcntl(stdout_pipe[0], F_SETFL, fcntl(stdout_pipe[0], F_GETFL) | O_NONBLOCK);
    fcntl(stderr_pipe[0], F_SETFL, fcntl(stderr_pipe[0], F_GETFL) | O_NONBLOCK);

    ChildResult result;
    bool stdout_open = true;
    bool stderr_open = true;
    bool child_done = false;
    int status = 0;
    const auto deadline =
        started + std::chrono::seconds(std::max(timeout_seconds, 1));
    while (!child_done || stdout_open || stderr_open) {
        drain_fd(stdout_pipe[0], result.stdout_text, stdout_open);
        drain_fd(stderr_pipe[0], result.stderr_text, stderr_open);
        if (!child_done) {
            const pid_t waited = waitpid(child, &status, WNOHANG);
            if (waited == child) {
                child_done = true;
            } else if (waited < 0 && errno != EINTR) {
                throw std::runtime_error("cannot wait for sandbox process");
            }
        }
        if (!child_done && std::chrono::steady_clock::now() >= deadline) {
            result.timed_out = true;
            kill(-child, SIGTERM);
            usleep(100000);
            kill(-child, SIGKILL);
            while (waitpid(child, &status, 0) < 0 && errno == EINTR) {}
            child_done = true;
        }
        if (!child_done || stdout_open || stderr_open) {
            struct pollfd poll_fds[2] = {
                {stdout_pipe[0], POLLIN, 0},
                {stderr_pipe[0], POLLIN, 0},
            };
            poll(poll_fds, 2, 20);
        }
    }
    result.duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - started)
                             .count();
    if (result.timed_out) {
        result.exit_code = 124;
    } else if (WIFEXITED(status)) {
        result.exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        result.exit_code = 128 + WTERMSIG(status);
    }
    return result;
}

inline fs::path make_temp_path(const std::string& prefix) {
    std::string pattern =
        (fs::temp_directory_path() / (prefix + "-XXXXXX")).string();
    std::vector<char> writable(pattern.begin(), pattern.end());
    writable.push_back('\0');
    const int fd = mkstemp(writable.data());
    if (fd < 0) {
        throw std::runtime_error("cannot allocate temporary path");
    }
    close(fd);
    fs::remove(writable.data());
    return fs::path(writable.data());
}

// Shared per-attempt dispatch state (Phase 7.5). Both the legacy flat
// call list and the control-flow tree executor dispatch through
// dispatch_single_call below, so capability re-checks, integrity
// re-checks, sandboxing, timeouts, and manifest writes cannot drift
// apart between the two paths. All pointers stay valid for the whole
// run_runtime_plan call (they alias its locals).
struct DispatchState {
    const RuntimePlan* plan = nullptr;
    const RuntimeOptions* options = nullptr;
    fs::path working;
    fs::path output_root;
    fs::path manifest_path;
    RuntimeManifest* manifest = nullptr;
    bool* all_ok = nullptr;
    std::size_t* execution_count = nullptr;
    std::set<std::string>* output_candidates = nullptr;
};

// Stamp a terminal outcome: wall-clock end + stdout hash (Phase 7.5).
// Called exactly once per attempt, at every terminal assignment.
inline void finalize_entry(ExecutionRecord& entry) {
    entry.end_utc = utc_now();
    entry.stdout_sha256 = sha256_bytes(entry.stdout_text);
}

// Dispatch one call end-to-end (moved verbatim from run_runtime_plan's
// loop in Phase 7.5; behavior identical, now shared). Appends the
// manifest entry as "started" BEFORE any spawn, then walks the denial
// chain (execution ceiling → capability → integrity → argument
// validation) and finally sandboxes with timeout. Never throws for
// outcome-level events (they become entry outcomes); filesystem-level
// persist failures still propagate to the caller.
inline void dispatch_single_call(const RuntimeCall& call,
                                 DispatchState& state) {
    RuntimeManifest& manifest = *state.manifest;
    const RuntimePlan& plan = *state.plan;
    const RuntimeOptions& options = *state.options;
    const fs::path& working = state.working;
    const fs::path& output_root = state.output_root;
    const fs::path& manifest_path = state.manifest_path;
    bool& all_ok = *state.all_ok;
    std::size_t& execution_count = *state.execution_count;
    std::set<std::string>& output_candidates = *state.output_candidates;

    ExecutionRecord record;
    record.function = call.function;
    record.capability = call.capability;
    record.script_sha256 = call.script_sha256;
    record.args = call.args;
    record.start_utc = utc_now();
    manifest.executions.push_back(record);
    ExecutionRecord& entry = manifest.executions.back();
    persist_manifest(manifest_path, manifest);

    if (execution_count >= options.max_executions) {
        entry.outcome = "ceiling_denied";
        entry.stderr_text = "runtime execution ceiling reached";
        all_ok = false;
        finalize_entry(entry);
        persist_manifest(manifest_path, manifest);
        return;
    }
    ++execution_count;
    if (!capability_allowed(plan.allowed_capabilities, call.capability)) {
        entry.outcome = "capability_denied";
        entry.stderr_text = "runtime capability re-check denied '" +
                            call.capability + "'";
        all_ok = false;
        finalize_entry(entry);
        persist_manifest(manifest_path, manifest);
        return;
    }
    if (sha256_bytes(call.script_source) != call.script_sha256) {
        entry.outcome = "integrity_denied";
        entry.stderr_text = "embedded script SHA-256 mismatch";
        all_ok = false;
        finalize_entry(entry);
        persist_manifest(manifest_path, manifest);
        return;
    }

    const fs::path sandbox_root = make_temp_path("jocky-sandbox");
    bool args_valid = true;
    std::vector<std::string> values;
    std::vector<std::pair<fs::path, std::string>> input_snapshots;
    std::vector<std::pair<fs::path, fs::path>> staged_outputs;
    try {
        prepare_isolated_root(sandbox_root, call.script_source);
        std::set<std::string> staged_destinations;
        auto stage_destination = [&](const fs::path& destination) {
            const fs::path staged =
                staged_output_path(output_root, destination);
            if (staged_destinations.insert(destination.string()).second) {
                staged_outputs.push_back({staged, destination});
                fs::create_directories(
                    (sandbox_root / staged.relative_path()).parent_path());
            }
            return staged.string();
        };
        for (const std::string& declared : plan.declared_outputs) {
            stage_destination(canonical_path(declared, working));
        }

        std::size_t input_index = 0;
        for (const RuntimeArgument& arg : call.args) {
            if (!valid_runtime_value(arg)) {
                args_valid = false;
                entry.stderr_text =
                    "runtime argument validation failed for '" + arg.name +
                    "' as " + arg.declared_type;
                break;
            }
            if (arg.declared_type != "path") {
                values.push_back(arg.value);
                continue;
            }
            if (arg.value.empty()) {
                values.emplace_back();
                continue;
            }
            const fs::path value_path = canonical_path(arg.value, working);
            if (path_is_within(value_path, output_root)) {
                output_candidates.insert(value_path.string());
                values.push_back(stage_destination(value_path));
            } else {
                const fs::path child_path =
                    snapshot_input(value_path, sandbox_root, input_index++);
                const fs::path host_snapshot =
                    sandbox_root / child_path.relative_path();
                input_snapshots.push_back(
                    {host_snapshot,
                     hash_artifact("", "", host_snapshot).sha256});
                values.push_back(child_path.string());
            }
        }
    } catch (const std::exception& ex) {
        args_valid = false;
        entry.stderr_text = ex.what();
    }
    if (!args_valid) {
        entry.outcome = "validation_denied";
        all_ok = false;
        fs::remove_all(sandbox_root);
        finalize_entry(entry);
        persist_manifest(manifest_path, manifest);
        return;
    }

    try {
        const ChildResult child =
            spawn_sandboxed(options, values, call.timeout_seconds,
                              sandbox_root);
        entry.exit_code = child.exit_code;
        entry.timed_out = child.timed_out;
        entry.duration_ms = child.duration_ms;
        entry.stdout_text = child.stdout_text;
        entry.stderr_text = child.stderr_text;
        entry.outcome = child.timed_out
                            ? "timeout"
                            : (child.exit_code == 0 ? "success" : "failure");
        bool input_changed = false;
        for (const auto& [snapshot, before_sha256] : input_snapshots) {
            try {
                if (hash_artifact("", "", snapshot).sha256 != before_sha256) {
                    input_changed = true;
                }
            } catch (const std::exception&) {
                input_changed = true;
            }
        }
        if (input_changed) {
            entry.outcome = "evidence_write_denied";
            if (!entry.stderr_text.empty()) entry.stderr_text += "\n";
            entry.stderr_text +=
                "isolated evidence snapshot was modified; outputs discarded";
            all_ok = false;
        } else if (entry.outcome == "success") {
            for (const auto& [staged, destination] : staged_outputs) {
                promote_output(sandbox_root, staged, destination);
            }
        } else {
            all_ok = false;
        }
    } catch (const std::exception& ex) {
        entry.outcome = "dispatcher_failure";
        entry.stderr_text = ex.what();
        all_ok = false;
    }
    fs::remove_all(sandbox_root);
    finalize_entry(entry);
    persist_manifest(manifest_path, manifest);
}

// Execution-time control-flow driver (Phase 7.5). Defined in
// runtime/control_flow_executor.hpp; declared here so run_runtime_plan
// can branch to it without a circular include. Returns true when the
// tree walked to completion; false on fail-closed abort (the abort is
// already manifest-logged with all_ok set false).
inline bool execute_plan_tree(const RuntimePlan& plan, DispatchState& state);

}  // namespace runtime_detail

inline DispatchResult run_runtime_plan(const RuntimePlan& plan,
                                       RuntimeOptions options) {
    namespace fs = std::filesystem;
    using namespace runtime_detail;

    DispatchResult result;
    RuntimeManifest& manifest = result.manifest;
    manifest.case_id = plan.case_id;
    manifest.program_sha256 = plan.program_sha256;
    manifest.allowed_capabilities = plan.allowed_capabilities;
    manifest.run_start_utc = utc_now();

    if (options.working_directory.empty()) {
        options.working_directory = fs::current_path().string();
    }
    if (options.executable_path.empty()) {
        options.executable_path = self_executable();
    }
    const fs::path working =
        canonical_path(options.working_directory, fs::current_path());
    fs::create_directories(working / options.output_root);
    const fs::path output_root =
        canonical_path(options.output_root, working);
    const fs::path manifest_path =
        canonical_path(options.manifest_path, working);
    fs::create_directories(output_root / ".jocky-tmp");
    if (!path_is_within(manifest_path, output_root)) {
        throw std::runtime_error("manifest path must be inside output root");
    }

    auto finish = [&](const std::string& status, int exit_code) {
        manifest.status = status;
        manifest.run_end_utc = utc_now();
        persist_manifest(manifest_path, manifest);
        result.exit_code = exit_code;
        return result;
    };

    try {
        for (const RuntimeEvidence& evidence : plan.evidence) {
            const fs::path path = canonical_path(evidence.path, working);
            if (path_is_within(path, output_root) ||
                path_is_within(output_root, path)) {
                throw std::runtime_error(
                    "output root overlaps evidence path '" + path.string() +
                    "'");
            }
            manifest.inputs.push_back(
                hash_artifact(evidence.name, evidence.adapter, path));
        }
        for (const std::string& output : plan.declared_outputs) {
            const fs::path path = canonical_path(output, working);
            if (!path_is_within(path, output_root)) {
                throw std::runtime_error(
                    "declared output escapes output root: '" + path.string() +
                    "'");
            }
        }
    } catch (const std::exception& ex) {
        manifest.errors.push_back(ex.what());
        return finish("preflight_failed", 1);
    }
    persist_manifest(manifest_path, manifest);

    bool all_ok = true;
    std::size_t execution_count = 0;
    std::set<std::string> output_candidates(plan.declared_outputs.begin(),
                                            plan.declared_outputs.end());
    DispatchState state;
    state.plan = &plan;
    state.options = &options;
    state.working = working;
    state.output_root = output_root;
    state.manifest_path = manifest_path;
    state.manifest = &manifest;
    state.all_ok = &all_ok;
    state.execution_count = &execution_count;
    state.output_candidates = &output_candidates;
    if (!plan.program_source.empty()) {
        // Phase 7.5 tree mode: walk the real statement tree (branch
        // selection, real loops, while ceiling). An empty source keeps
        // the legacy flat dispatch (hand-built plans, Phase 7 tests).
        execute_plan_tree(plan, state);
    } else {
        for (const RuntimeCall& call : plan.calls) {
            dispatch_single_call(call, state);
        }
    }

    for (const std::string& output : output_candidates) {
        try {
            const fs::path path = canonical_path(output, working);
            if (fs::exists(path)) {
                manifest.outputs.push_back(hash_artifact("", "", path));
            }
        } catch (const std::exception& ex) {
            manifest.errors.push_back(ex.what());
            all_ok = false;
        }
    }
    return finish(all_ok ? "success" : "failed", all_ok ? 0 : 1);
}

}  // namespace jocky
