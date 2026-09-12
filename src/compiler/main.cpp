// jockyc — Phase 6 ahead-of-time packager for the JOCKY forensic DSL.
// Linux/WSL scope: validates, gates, shakes, embeds, and invokes the host C++
// compiler without a shell. The generated artifact inventories/extracts its
// embedded scripts; execution is deliberately Phase 7.

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
#include "jocky/lexer/lexer.hpp"
#include "jocky/parser/parser.hpp"
#include "jocky/policy/capability_gate.hpp"
#include "jocky/semantic/bound_checker.hpp"
#include "jocky/semantic/call_resolver.hpp"
#include "jocky/semantic/case_binder.hpp"
#include "jocky/stdlib/script_metadata.hpp"

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

void write_runner(const std::string& path, const jocky::EmbedResult& embedded,
                  const std::string& case_name) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        throw std::runtime_error("cannot create generated source '" + path +
                                 "'");
    }
    out << "#include <cstring>\n#include <iostream>\n#include <string>\n";
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
    out << "int main(int argc, char** argv) {\n"
           "  if (argc == 3 && std::string(argv[1]) == \"--extract\") {\n"
           "    for (unsigned long i = 0; i < unit_count; ++i) { const Unit& u = units[i]; "
           "if (std::string(argv[2]) == u.function) { "
           "std::cout.write(reinterpret_cast<const char*>(u.data), "
           "static_cast<std::streamsize>(u.size)); return 0; } }\n"
           "    std::cerr << \"unknown embedded function: \" << argv[2] << \"\\n\"; return 2;\n"
           "  }\n"
           "  if (argc != 1 && !(argc == 2 && std::string(argv[1]) == \"--list-embedded\")) {\n"
           "    std::cerr << \"usage: standalone [--list-embedded] [--extract function]\\n\"; return 2;\n"
           "  }\n"
           "  std::cout << \"JOCKY standalone case \" << "
        << cpp_string(case_name)
        << " << \" scripts=\" << unit_count << \"\\n\";\n"
           "  for (unsigned long i = 0; i < unit_count; ++i) { const Unit& u = units[i]; "
           "std::cout << \"EMBEDDED \" << u.function << "
           "\" sha256=\" << u.sha256 << \" bytes=\" << u.size << \"\\n\"; }\n"
           "  return 0;\n}\n";
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
        compiler, "-std=c++20", "-O2", generated, "-o", output};
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

        write_runner(generated, embedded, gate.case_name);
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
