#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include <unistd.h>

#include "jocky/runtime/control_flow_executor.hpp"

namespace fs = std::filesystem;

namespace {

void write_file(const fs::path& path, const std::string& text) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << text;
}

std::string read_file(const fs::path& path) {
    return jocky::read_binary_file(path.string());
}

jocky::RuntimeCall make_call(const std::string& function,
                             const std::string& capability,
                             const std::string& script,
                             int timeout,
                             std::vector<jocky::RuntimeArgument> args = {}) {
    jocky::RuntimeCall call;
    call.function = function;
    call.capability = capability;
    call.script_source = script;
    call.script_sha256 = jocky::sha256_bytes(script);
    call.timeout_seconds = timeout;
    call.args = std::move(args);
    return call;
}

jocky::DispatchResult run_one(const fs::path& root,
                              const std::string& name,
                              jocky::RuntimePlan plan,
                              std::size_t ceiling = 1000) {
    const fs::path out = root / name;
    jocky::RuntimeOptions options;
    options.executable_path = jocky::runtime_detail::self_executable();
    options.working_directory = root.string();
    options.output_root = out.string();
    options.manifest_path = (out / "manifest.json").string();
    options.max_executions = ceiling;
    return jocky::run_runtime_plan(plan, options);
}

bool contains(const fs::path& path, const std::string& needle) {
    return read_file(path).find(needle) != std::string::npos;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc > 1 && std::string(argv[1]) == "--jocky-sandbox-child") {
        return jocky::runtime_detail::sandbox_child_main(argc, argv);
    }

    const fs::path root = fs::temp_directory_path() /
                          ("jocky-phase7-test-" + std::to_string(getpid()));
    fs::remove_all(root);
    fs::create_directories(root);
    const fs::path evidence = root / "evidence.txt";
    write_file(evidence, "original evidence\n");

    jocky::RuntimePlan base;
    base.case_id = "phase7";
    base.program_sha256 = jocky::sha256_bytes("phase7 fixture");
    base.allowed_capabilities = {"test.execute"};
    base.evidence.push_back({"ev", "file", evidence.string()});

    const std::string literal = "$(touch /tmp/jocky-argv-injection); spaced";
    jocky::RuntimePlan success = base;
    const fs::path success_out = root / "success" / "result.txt";
    success.declared_outputs.push_back(success_out.string());
    success.calls.push_back(make_call(
        "jky_recon_success_fixture", "test.execute",
        "#!/bin/bash\nset -euo pipefail\nprintf '%s' \"$1\" > \"$2\"\nprintf 'captured-out\\n'\nprintf 'captured-err\\n' >&2\n",
        3, {{"value", "string", literal, true},
            {"out", "path", success_out.string(), true}}));
    const jocky::DispatchResult ok = run_one(root, "success", success);
    if (ok.exit_code != 0 || read_file(success_out) != literal ||
        fs::exists("/tmp/jocky-argv-injection") ||
        !contains(root / "success/manifest.json", "\"outcome\":\"success\"") ||
        !contains(root / "success/manifest.json", "captured-out") ||
        !contains(root / "success/manifest.json", "captured-err")) {
        std::cerr << "success/argv/capture test failed\n";
        return 1;
    }

    const std::string registry_source =
        "#!/bin/bash\nprintf 'registry-ok\\n'\n";
    const fs::path registry_path = root / "registry-script.sh";
    write_file(registry_path, registry_source);
    jocky::ScriptMetadata registry_metadata;
    registry_metadata.function = "jky_recon_registry_fixture";
    registry_metadata.capability = "test.execute";
    registry_metadata.timeout_seconds = 3;
    registry_metadata.script_path = registry_path.string();
    registry_metadata.source_sha256 = jocky::sha256_bytes(registry_source);
    jocky::RuntimePlan filesystem_registry = base;
    filesystem_registry.calls.push_back(jocky::load_registry_runtime_call(
        registry_metadata, {}));
    const jocky::DispatchResult registry_result =
        run_one(root, "registry", filesystem_registry);
    if (registry_result.exit_code != 0 ||
        !contains(root / "registry/manifest.json", "registry-ok") ||
        !contains(root / "registry/manifest.json",
                  "\"outcome\":\"success\"")) {
        std::cerr << "filesystem registry dispatch test failed\n";
        return 1;
    }

    write_file(registry_path, "#!/bin/bash\nexit 9\n");
    jocky::RuntimePlan stale_registry = base;
    stale_registry.calls.push_back(jocky::load_registry_runtime_call(
        registry_metadata, {}));
    const jocky::DispatchResult stale_registry_result =
        run_one(root, "registry-stale", stale_registry);
    if (stale_registry_result.exit_code == 0 ||
        !contains(root / "registry-stale/manifest.json",
                  "\"outcome\":\"integrity_denied\"")) {
        std::cerr << "filesystem registry drift test failed\n";
        return 1;
    }

    jocky::RuntimePlan failure = base;
    failure.calls.push_back(make_call(
        "jky_recon_failure_fixture", "test.execute",
        "#!/bin/bash\nprintf 'failed\\n' >&2\nexit 7\n", 3));
    const jocky::DispatchResult failed = run_one(root, "failure", failure);
    if (failed.exit_code == 0 ||
        !contains(root / "failure/manifest.json", "\"exit_code\":7") ||
        !contains(root / "failure/manifest.json", "\"outcome\":\"failure\"")) {
        std::cerr << "non-zero test failed\n";
        return 1;
    }

    jocky::RuntimePlan timeout = base;
    timeout.calls.push_back(make_call(
        "jky_recon_timeout_fixture", "test.execute",
        "#!/bin/bash\nsleep 5\n", 1));
    const jocky::DispatchResult timed = run_one(root, "timeout", timeout);
    if (timed.exit_code == 0 ||
        !contains(root / "timeout/manifest.json", "\"timed_out\":true") ||
        !contains(root / "timeout/manifest.json", "\"outcome\":\"timeout\"")) {
        std::cerr << "timeout test failed\n";
        return 1;
    }

    jocky::RuntimePlan denied = base;
    denied.allowed_capabilities.clear();
    const fs::path denied_marker = root / "denied/should-not-exist";
    denied.calls.push_back(make_call(
        "jky_recon_denied_fixture", "test.execute",
        "#!/bin/bash\ntouch \"$1\"\n", 3,
        {{"marker", "path", denied_marker.string(), true}}));
    const jocky::DispatchResult denial = run_one(root, "denied", denied);
    if (denial.exit_code == 0 || fs::exists(denied_marker) ||
        !contains(root / "denied/manifest.json",
                  "\"outcome\":\"capability_denied\"")) {
        std::cerr << "runtime capability denial test failed\n";
        return 1;
    }

    jocky::RuntimePlan readonly = base;
    readonly.calls.push_back(make_call(
        "jky_recon_evidence_write_fixture", "test.execute",
        "#!/bin/bash\nprintf changed > \"$1\"\n", 3,
        {{"evidence", "path", evidence.string(), true}}));
    const jocky::DispatchResult protected_result =
        run_one(root, "readonly", readonly);
    if (protected_result.exit_code == 0 ||
        read_file(evidence) != "original evidence\n" ||
        !contains(root / "readonly/manifest.json",
                  "\"outcome\":\"evidence_write_denied\"")) {
        std::cerr << "read-only evidence test failed\n";
        return 1;
    }

    jocky::RuntimePlan integrity = base;
    jocky::RuntimeCall corrupt = make_call(
        "jky_recon_integrity_fixture", "test.execute",
        "#!/bin/bash\nexit 0\n", 3);
    corrupt.script_sha256 = std::string(64, '0');
    integrity.calls.push_back(std::move(corrupt));
    const jocky::DispatchResult integrity_result =
        run_one(root, "integrity", integrity);
    if (integrity_result.exit_code == 0 ||
        !contains(root / "integrity/manifest.json",
                  "\"outcome\":\"integrity_denied\"")) {
        std::cerr << "script integrity test failed\n";
        return 1;
    }

    jocky::RuntimePlan invalid = base;
    invalid.calls.push_back(make_call(
        "jky_recon_invalid_fixture", "test.execute",
        "#!/bin/bash\ntouch \"$1\"\n", 3,
        {{"count", "int", "not-an-int", true}}));
    const jocky::DispatchResult invalid_result =
        run_one(root, "invalid", invalid);
    if (invalid_result.exit_code == 0 ||
        !contains(root / "invalid/manifest.json",
                  "\"outcome\":\"validation_denied\"")) {
        std::cerr << "runtime validation test failed\n";
        return 1;
    }

    jocky::RuntimePlan escape = base;
    escape.declared_outputs.push_back((root / "escape.txt").string());
    const jocky::DispatchResult escaped = run_one(root, "escape", escape);
    if (escaped.exit_code == 0 ||
        !contains(root / "escape/manifest.json", "preflight_failed") ||
        fs::exists(root / "escape.txt")) {
        std::cerr << "output escape test failed\n";
        return 1;
    }

    jocky::RuntimePlan ceiling = base;
    ceiling.calls.push_back(make_call(
        "jky_recon_first_fixture", "test.execute",
        "#!/bin/bash\nexit 0\n", 3));
    ceiling.calls.push_back(make_call(
        "jky_recon_second_fixture", "test.execute",
        "#!/bin/bash\nexit 0\n", 3));
    const jocky::DispatchResult ceiling_result =
        run_one(root, "ceiling", ceiling, 1);
    if (ceiling_result.exit_code == 0 ||
        !contains(root / "ceiling/manifest.json",
                  "\"outcome\":\"ceiling_denied\"")) {
        std::cerr << "execution ceiling test failed\n";
        return 1;
    }

    std::cout << "PHASE7_RUNTIME_TEST_ROOT=" << root << "\n";
    return 0;
}
