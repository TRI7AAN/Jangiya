// jockyc — ahead-of-time packager for the JOCKY forensic DSL.
// Linux/WSL scope: validates, gates, shakes, embeds, and invokes the host C++
// compiler without a shell. The generated artifact inventories, extracts, and
// dispatches embedded scripts through the Phase 7 isolated runtime.

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "jocky/compiler/embedder.hpp"
#include "jocky/crypto/sha256.hpp"
#include "jocky/lexer/lexer.hpp"
#include "jocky/parser/parser.hpp"
#include "jocky/policy/capability_gate.hpp"
#include "jocky/semantic/bound_checker.hpp"
#include "jocky/semantic/call_resolver.hpp"
#include "jocky/semantic/case_binder.hpp"
#include "jocky/stdlib/script_metadata.hpp"

#ifndef JOCKY_INCLUDE_DIR
#define JOCKY_INCLUDE_DIR "include"
#endif

namespace {

struct Options {
    std::string source;
    std::string output = "./a.out";
    std::string registry = "stat_scripts/";
    bool list_used = false;
};

Options parse_options(int argc, char** argv) {
    if (argc < 2) {
        throw std::runtime_error("missing .jky input");
    }
    Options options;
    options.source = argv[1];
    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "-o" && i + 1 < argc) {
            options.output = argv[++i];
        } else if (arg == "--registry" && i + 1 < argc) {
            options.registry = argv[++i];
        } else if (arg == "--list-used") {
            options.list_used = true;
        } else {
            throw std::runtime_error("unknown or incomplete option '" + arg +
                                     "'");
        }
    }
    return options;
}

jocky::Program load_program(const std::string& path) {
    const std::string source = jocky::read_binary_file(path);
    jocky::Lexer lexer(source);
    jocky::Parser parser(lexer.tokenize());
    return parser.parse_program();
}

std::string cpp_string(const std::string& text) {
    std::ostringstream out;
    out << '"';
    for (unsigned char c : text) {
        switch (c) {
            case '\\': out << "\\\\"; break;
            case '"': out << "\\\""; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (c < 0x20U || c >= 0x7fU) {
                    const char hex[] = "0123456789abcdef";
                    out << "\\x" << hex[c >> 4U] << hex[c & 0x0fU];
                } else {
                    out << static_cast<char>(c);
                }
        }
    }
    out << '"';
    return out.str();
}

void write_byte_array(std::ostream& out, const std::string& name,
                      const std::string& source) {
    out << "static const unsigned char " << name << "[] = {";
    if (source.empty()) {
        out << "0";
    } else {
        const char hex[] = "0123456789abcdef";
        for (std::size_t i = 0; i < source.size(); ++i) {
            const unsigned char c = static_cast<unsigned char>(source[i]);
            if (i != 0) out << ",";
            out << "0x" << hex[c >> 4U] << hex[c & 0x0fU];
        }
    }
    out << "};\n";
}

std::vector<std::string> collect_declared_outputs(
    const jocky::Program& program) {
    std::vector<std::string> outputs;
    auto collect = [&](const jocky::PipelineStmt& statement) {
        for (const jocky::PipelineStep& step : statement.expr.steps) {
            if (step.op.kind == jocky::PipelineOp::Kind::Write) {
                outputs.push_back(step.op.write_path);
            }
        }
    };
    for (const jocky::RuleDecl& rule : program.rules) {
        jocky::walker::for_each_pipeline_stmt_in_block(rule.body, collect);
    }
    for (const jocky::InvestigationDecl& investigation :
         program.investigations) {
        jocky::walker::for_each_pipeline_stmt_in_block(investigation.body,
                                                       collect);
    }
    return outputs;
}

std::size_t embedded_index(const jocky::EmbedResult& embedded,
                           const std::string& function) {
    for (std::size_t i = 0; i < embedded.scripts.size(); ++i) {
        if (embedded.scripts[i].metadata.function == function) return i;
    }
    throw std::runtime_error("authorized function '" + function +
                             "' missing from embedded closure");
}

void write_runner(const std::string& path, const jocky::EmbedResult& embedded,
                  const jocky::Program& program,
                  const jocky::BoundProgram& bound,
                  const jocky::GateResult& gate,
                  const std::string& source_sha256) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        throw std::runtime_error("cannot create generated source '" + path +
                                 "'");
    }
    out << "#include <filesystem>\n#include <iostream>\n"
           "#include <string>\n#include <vector>\n"
           "#include \"jocky/runtime/dispatcher.hpp\"\n";
    for (std::size_t i = 0; i < embedded.scripts.size(); ++i) {
        write_byte_array(out, "script_" + std::to_string(i),
                         embedded.scripts[i].source);
    }
    out << "struct Unit { const char* function; const char* sha256; "
           "const unsigned char* data; unsigned long size; };\n";
    out << "static const Unit units[] = {\n";
    if (embedded.scripts.empty()) {
        out << "{nullptr,nullptr,nullptr,0UL},\n";
    } else {
        for (std::size_t i = 0; i < embedded.scripts.size(); ++i) {
            const auto& script = embedded.scripts[i];
            out << "{" << cpp_string(script.metadata.function) << ","
                << cpp_string(script.sha256) << ",script_" << i << ","
                << script.source.size() << "UL},\n";
        }
    }
    out << "};\nstatic constexpr unsigned long unit_count = "
        << embedded.scripts.size() << "UL;\n";

    out << "static jocky::RuntimePlan build_plan() {\n"
           "  jocky::RuntimePlan plan;\n"
           "  plan.case_id = " << cpp_string(gate.case_name) << ";\n"
           "  plan.jky_sha256 = " << cpp_string(source_sha256) << ";\n";
    for (const std::string& capability :
         bound.bound_case.capabilities) {
        out << "  plan.allowed_capabilities.push_back("
            << cpp_string(capability) << ");\n";
    }
    for (const jocky::EvidenceDecl& evidence : program.evidence) {
        out << "  plan.evidence.push_back({"
            << cpp_string(evidence.name) << ","
            << cpp_string(evidence.adapter) << ","
            << cpp_string(evidence.path) << "});\n";
    }
    for (const std::string& output : collect_declared_outputs(program)) {
        out << "  plan.declared_outputs.push_back(" << cpp_string(output)
            << ");\n";
    }
    for (const jocky::ResolvedCall& call : gate.authorized) {
        const std::size_t index = embedded_index(embedded, call.function);
        const auto& metadata = embedded.scripts[index].metadata;
        out << "  { jocky::RuntimeCall call; call.function = "
            << cpp_string(call.function) << "; call.capability = "
            << cpp_string(call.capability) << "; call.script_sha256 = "
            << cpp_string(embedded.scripts[index].sha256)
            << "; call.script_source.assign("
               "reinterpret_cast<const char*>(script_"
            << index << "), " << embedded.scripts[index].source.size()
            << "UL); call.timeout_seconds = " << metadata.timeout_seconds
            << ";\n";
        for (const jocky::ResolvedArg& arg : call.args) {
            out << "    call.args.push_back({" << cpp_string(arg.name) << ","
                << cpp_string(arg.declared_type) << ","
                << cpp_string(arg.value) << ","
                << (arg.concrete ? "true" : "false") << "});\n";
        }
        out << "    plan.calls.push_back(std::move(call)); }\n";
    }
    out << "  return plan;\n}\n";

    out << R"CPP(
int main(int argc, char** argv) {
  if (argc > 1 && std::string(argv[1]) == "--jocky-sandbox-child") {
    return jocky::runtime_detail::sandbox_child_main(argc, argv);
  }
  if (argc == 3 && std::string(argv[1]) == "--extract") {
    for (unsigned long i = 0; i < unit_count; ++i) {
      const Unit& u = units[i];
      if (std::string(argv[2]) == u.function) {
        std::cout.write(reinterpret_cast<const char*>(u.data),
                        static_cast<std::streamsize>(u.size));
        return 0;
      }
    }
    std::cerr << "unknown embedded function: " << argv[2] << "\n";
    return 2;
  }
  if (argc > 1 && std::string(argv[1]) == "run") {
    jocky::RuntimeOptions options;
    options.executable_path = jocky::runtime_detail::self_executable();
    options.working_directory = std::filesystem::current_path().string();
    options.manifest_path.clear();
    for (int i = 2; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--output-root" && i + 1 < argc) {
        options.output_root = argv[++i];
      } else if (arg == "--manifest" && i + 1 < argc) {
        options.manifest_path = argv[++i];
      } else if (arg == "--max-executions" && i + 1 < argc) {
        options.max_executions =
            static_cast<std::size_t>(std::stoull(argv[++i]));
      } else {
        std::cerr << "unknown or incomplete run option: " << arg << "\n";
        return 2;
      }
    }
    if (options.manifest_path.empty()) {
      options.manifest_path =
          (std::filesystem::path(options.output_root) / "manifest.json")
              .string();
    }
    const jocky::DispatchResult result =
        jocky::run_runtime_plan(build_plan(), options);
    std::cout << "MANIFEST " << options.manifest_path
              << " status=" << result.manifest.status << "\n";
    return result.exit_code;
  }
  if (argc != 1 &&
      !(argc == 2 && std::string(argv[1]) == "--list-embedded")) {
    std::cerr << "usage: standalone [--list-embedded] "
                 "[--extract function] [run [--output-root dir] "
                 "[--manifest path] [--max-executions n]]\n";
    return 2;
  }
  std::cout << "JOCKY standalone case " << )CPP"
        << cpp_string(gate.case_name)
        << R"CPP( << " scripts=" << unit_count << "\n";
  for (unsigned long i = 0; i < unit_count; ++i) {
    const Unit& u = units[i];
    std::cout << "EMBEDDED " << u.function << " sha256=" << u.sha256
              << " bytes=" << u.size << "\n";
  }
  return 0;
}
)CPP";
    if (!out) {
        throw std::runtime_error("cannot write generated source '" + path +
                                 "'");
    }
}

int compile_runner(const std::string& generated, const std::string& output) {
    const char* configured = std::getenv("JOCKY_CXX");
    const std::string compiler =
        (configured != nullptr && configured[0] != '\0') ? configured : "c++";
    const std::vector<std::string> owned = {
        compiler, "-std=c++20", "-O2", "-I", JOCKY_INCLUDE_DIR,
        generated, "-o", output};
    std::vector<char*> args;
    for (const std::string& arg : owned) {
        args.push_back(const_cast<char*>(arg.c_str()));
    }
    args.push_back(nullptr);

    const pid_t child = fork();
    if (child < 0) {
        throw std::runtime_error("cannot fork compiler: " +
                                 std::string(std::strerror(errno)));
    }
    if (child == 0) {
        execvp(args[0], args.data());
        std::fprintf(stderr, "error: cannot execute compiler '%s': %s\n",
                     args[0], std::strerror(errno));
        _exit(127);
    }
    int status = 0;
    if (waitpid(child, &status, 0) < 0) {
        throw std::runtime_error("cannot wait for compiler: " +
                                 std::string(std::strerror(errno)));
    }
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}

void print_denials(const std::string& source, const jocky::GateResult& gate) {
    for (const auto& violation : gate.violations) {
        std::cerr << source << ":" << violation.line << ":" << violation.col
                  << ": call '" << violation.function
                  << "' requires capability '" << violation.capability
                  << "' not granted by case '" << violation.case_name << "'\n";
    }
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const Options options = parse_options(argc, argv);
        jocky::Program program = load_program(options.source);
        jocky::ScanResult scanned = jocky::scan_registry_dir(options.registry);
        if (!scanned.rejections.empty()) {
            for (const std::string& rejection : scanned.rejections) {
                std::cerr << "REJECT " << rejection << "\n";
            }
            throw std::runtime_error("registry scan rejected entries");
        }
        jocky::ResolvedProgram resolved =
            jocky::resolve_program(program, scanned.registry);
        jocky::check_bounds(program);
        jocky::BoundProgram bound =
            jocky::bind_program(program, std::move(resolved));
        jocky::GateResult gate = jocky::check_gate(bound);
        if (!gate.allowed) {
            print_denials(options.source, gate);
            throw std::runtime_error("capability gate denied compilation");
        }
        const jocky::ShakeResult shaken =
            jocky::resolve_scripts(gate, scanned.registry);
        if (options.list_used) {
            for (const auto& script : shaken.scripts) {
                std::cout << script.metadata.function << " "
                          << script.metadata.source_sha256 << "\n";
            }
            return 0;
        }
        const jocky::EmbedResult embedded = jocky::embed_scripts(shaken);

        std::string pattern =
            (std::filesystem::temp_directory_path() / "jockyc-XXXXXX.cpp")
                .string();
        std::vector<char> writable(pattern.begin(), pattern.end());
        writable.push_back('\0');
        const int fd = mkstemps(writable.data(), 4);
        if (fd < 0) {
            throw std::runtime_error("cannot allocate generated source");
        }
        close(fd);
        const std::string generated = writable.data();
        struct TempGuard {
            std::string path;
            ~TempGuard() { std::remove(path.c_str()); }
        } guard{generated};

        const std::string source_sha256 =
            jocky::sha256_file(options.source);
        write_runner(generated, embedded, program, bound, gate,
                     source_sha256);
        const int compile_status = compile_runner(generated, options.output);
        if (compile_status != 0) {
            std::cerr << "error: host compiler exited " << compile_status
                      << "\n";
            return 1;
        }
        std::cout << "BUILT " << options.output << " with "
                  << embedded.scripts.size() << " embedded script(s)\n";
        for (const auto& script : embedded.scripts) {
            std::cout << "  " << script.metadata.function << " "
                      << script.sha256 << "\n";
        }
        return 0;
    } catch (const jocky::LexError& ex) {
        std::cerr << "error: " << ex.line << ":" << ex.col << ": "
                  << ex.what() << "\n";
    } catch (const jocky::ParseError& ex) {
        std::cerr << "error: " << ex.line << ":" << ex.col << ": "
                  << ex.what() << "\n";
    } catch (const jocky::SemanticError& ex) {
        std::cerr << "error: " << ex.line << ":" << ex.col << ": "
                  << ex.what() << "\n";
    } catch (const jocky::BoundCheckError& ex) {
        std::cerr << "error: " << ex.line << ":" << ex.col << ": "
                  << ex.what() << " (bound check)\n";
    } catch (const jocky::BindingError& ex) {
        std::cerr << "error: " << ex.what() << " (case binding)\n";
    } catch (const jocky::ShakeError& ex) {
        std::cerr << "error: " << ex.what() << " (shake)\n";
    } catch (const jocky::EmbedError& ex) {
        std::cerr << "error: " << ex.what() << " (embed)\n";
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << "\n";
    }
    std::cerr << "usage: jockyc <file.jky> [-o output] "
                 "[--registry dir] [--list-used]\n";
    return 1;
}
