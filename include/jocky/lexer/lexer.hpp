#pragma once
// JOCKY lexer — tokenizer for .jky source files (Phase 1 skeleton,
// control-flow keywords added Phase 5.5: if/else/for/while/int;
// full C/C++/Java keyword sets reserved Phase 6: every word below lexes
// as Keyword and can never be a binding/field/function name. Most have
// NO grammar rule (reserved future syntax space, not usable today) —
// see wiki/grammar.md §1. Deliberately NOT reserved: true/false (lex as
// BoolLit via scan_word precedence) and null (a literal, would need
// literal semantics — flagged future, not silently added).
// Produces Identifier, Keyword, StringLit, IntLit, FloatLit, BoolLit,
// Symbol, and EndOfFile tokens with 1-based line/column tracking.
// `#` starts a line comment. Unknown escapes and unterminated strings
// raise LexError carrying line/col.

#include <cctype>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace jocky {

enum class TokenKind {
    Identifier,
    Keyword,
    StringLit,
    IntLit,
    FloatLit,
    BoolLit,
    Symbol,
    EndOfFile,
};

inline const char* token_kind_name(TokenKind kind) {
    switch (kind) {
        case TokenKind::Identifier: return "Identifier";
        case TokenKind::Keyword: return "Keyword";
        case TokenKind::StringLit: return "StringLit";
        case TokenKind::IntLit: return "IntLit";
        case TokenKind::FloatLit: return "FloatLit";
        case TokenKind::BoolLit: return "BoolLit";
        case TokenKind::Symbol: return "Symbol";
        case TokenKind::EndOfFile: return "EndOfFile";
    }
    return "?";
}

struct Token {
    TokenKind kind = TokenKind::EndOfFile;
    std::string lexeme;
    int line = 1;
    int col = 1;
};

// Thrown on any lexical failure. Carries 1-based line/col of the
// offending character. Mirrors ParseError's style (see parser.hpp).
class LexError : public std::runtime_error {
public:
    int line;
    int col;
    LexError(int line, int col, const std::string& message)
        : std::runtime_error(message), line(line), col(col) {}
};

class Lexer {
public:
    explicit Lexer(std::string source) : src_(std::move(source)) {}

    std::vector<Token> tokenize() {
        tokens_.clear();
        pos_ = 0;
        line_ = 1;
        col_ = 1;
        while (!at_end()) {
            skip_trivia();
            if (at_end()) {
                break;
            }
            int tok_line = line_;
            int tok_col = col_;
            char c = peek();
            if (c == '"') {
                tokens_.push_back(scan_string(tok_line, tok_col));
            } else if (std::isdigit(static_cast<unsigned char>(c))) {
                tokens_.push_back(scan_number(tok_line, tok_col));
            } else if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
                tokens_.push_back(scan_word(tok_line, tok_col));
            } else {
                tokens_.push_back(scan_symbol(tok_line, tok_col));
            }
        }
        tokens_.push_back(Token{TokenKind::EndOfFile, "", line_, col_});
        return tokens_;
    }

    static const std::unordered_set<std::string>& keywords() {
        static const std::unordered_set<std::string> table = {
            "case",       "evidence",   "rule",      "investigate",
            "report",     "emit",       "where",     "select",
            "group_by",   "having",     "sort_by",   "limit",
            "correlate",  "within",     "on",        "as",
            "when",       "score",      "tag",       "call",
            "filter",     "write",      "let",       "pcap",
            "eventlog",   "directory",  "json",      "csv",
            "markdown",   "html",       "readonly",  "and",
            "or",         "not",        "in",        "contains",
            "contains_any",
            "if",         "else",       "for",       "while",
            "int",
            // C11 keywords (reserved; no grammar rules — future space).
            "auto",       "break",      "char",      "const",
            "continue",   "default",    "do",        "double",
            "enum",       "extern",     "float",     "goto",
            "inline",     "long",       "register",  "restrict",
            "return",     "short",      "signed",    "sizeof",
            "static",     "struct",     "switch",    "typedef",
            "union",      "unsigned",   "void",      "volatile",
            "_Alignas",   "_Alignof",   "_Atomic",   "_Bool",
            "_Complex",   "_Generic",   "_Imaginary","_Noreturn",
            "_Static_assert", "_Thread_local",
            // C++ keywords beyond C (reserved; no grammar rules).
            // (and/or/not already above; true/false stay BoolLit.)
            "asm",        "bool",       "catch",     "char8_t",
            "char16_t",   "char32_t",   "class",     "compl",
            "concept",    "consteval",  "constexpr", "constinit",
            "const_cast", "co_await",   "co_return", "co_yield",
            "decltype",   "delete",     "dynamic_cast", "explicit",
            "export",     "friend",     "mutable",   "namespace",
            "new",        "noexcept",   "nullptr",   "operator",
            "private",    "protected",  "public",    "reinterpret_cast",
            "requires",   "static_assert", "static_cast", "template",
            "this",       "thread_local", "throw",   "try",
            "typeid",     "typename",   "using",     "virtual",
            "wchar_t",    "and_eq",     "bitand",    "bitor",
            "not_eq",     "or_eq",      "xor",       "xor_eq",
            // Java keywords beyond C/C++ (reserved; no grammar rules).
            "abstract",   "assert",     "boolean",   "byte",
            "extends",    "final",      "finally",   "implements",
            "import",     "instanceof", "interface", "native",
            "package",    "strictfp",   "super",     "synchronized",
            "throws",     "transient",
        };
        return table;
    }

private:
    std::string src_;
    std::vector<Token> tokens_;
    std::size_t pos_ = 0;
    int line_ = 1;
    int col_ = 1;

    bool at_end() const { return pos_ >= src_.size(); }

    char peek() const { return at_end() ? '\0' : src_[pos_]; }

    char peek_next() const {
        return (pos_ + 1 >= src_.size()) ? '\0' : src_[pos_ + 1];
    }

    char advance() {
        char c = src_[pos_++];
        if (c == '\n') {
            ++line_;
            col_ = 1;
        } else {
            ++col_;
        }
        return c;
    }

    [[noreturn]] void fail(int line, int col, const std::string& message) {
        throw LexError(line, col, message);
    }

    void skip_trivia() {
        while (!at_end()) {
            char c = peek();
            if (c == ' ' || c == '\t' || c == '\r') {
                advance();
            } else if (c == '\n') {
                advance();
            } else if (c == '#') {
                while (!at_end() && peek() != '\n') {
                    advance();
                }
            } else {
                break;
            }
        }
    }

    Token scan_string(int tok_line, int tok_col) {
        advance();  // opening quote
        std::string value;
        while (!at_end()) {
            char c = advance();
            if (c == '"') {
                return Token{TokenKind::StringLit, value, tok_line, tok_col};
            }
            if (c == '\n') {
                fail(tok_line, tok_col, "unterminated string literal");
            }
            if (c == '\\') {
                if (at_end()) {
                    break;
                }
                char esc = advance();
                switch (esc) {
                    case '"': value += '"'; break;
                    case '\\': value += '\\'; break;
                    case 'n': value += '\n'; break;
                    case 't': value += '\t'; break;
                    case 'r': value += '\r'; break;
                    default:
                        fail(tok_line, tok_col,
                             std::string("unknown escape sequence '\\") + esc +
                                 "'");
                }
            } else {
                value += c;
            }
        }
        fail(tok_line, tok_col, "unterminated string literal");
    }

    Token scan_number(int tok_line, int tok_col) {
        std::size_t start = pos_;
        bool is_float = false;
        while (!at_end() &&
               std::isdigit(static_cast<unsigned char>(peek()))) {
            advance();
        }
        if (peek() == '.' &&
            std::isdigit(static_cast<unsigned char>(peek_next()))) {
            is_float = true;
            advance();  // consume '.'
            while (!at_end() &&
                   std::isdigit(static_cast<unsigned char>(peek()))) {
                advance();
            }
        }
        std::string lexeme = src_.substr(start, pos_ - start);
        return Token{is_float ? TokenKind::FloatLit : TokenKind::IntLit,
                     lexeme, tok_line, tok_col};
    }

    Token scan_word(int tok_line, int tok_col) {
        std::size_t start = pos_;
        while (!at_end() &&
               (std::isalnum(static_cast<unsigned char>(peek())) ||
                peek() == '_')) {
            advance();
        }
        std::string lexeme = src_.substr(start, pos_ - start);
        if (lexeme == "true" || lexeme == "false") {
            return Token{TokenKind::BoolLit, lexeme, tok_line, tok_col};
        }
        if (keywords().count(lexeme) != 0) {
            return Token{TokenKind::Keyword, lexeme, tok_line, tok_col};
        }
        return Token{TokenKind::Identifier, lexeme, tok_line, tok_col};
    }

    Token scan_symbol(int tok_line, int tok_col) {
        char c = peek();
        char n = peek_next();
        std::string two;
        two += c;
        two += n;
        if (two == "->" || two == "==" || two == "!=" || two == "<=" ||
            two == ">=") {
            advance();
            advance();
            return Token{TokenKind::Symbol, two, tok_line, tok_col};
        }
        static const std::string singles = "{}()[],;:|.=<>+-*/!";
        if (singles.find(c) != std::string::npos) {
            advance();
            return Token{TokenKind::Symbol, std::string(1, c), tok_line,
                         tok_col};
        }
        fail(tok_line, tok_col,
             std::string("unexpected character '") + c + "'");
    }
};

}  // namespace jocky
