#pragma once
// JOCKY parser — handwritten recursive descent over lexer tokens
// (Phase 1 skeleton). Implements the .jky grammar from
// wiki/06-api-contracts.md as amended for the Phase 1 pipeline
// operators (where/group_by/having/sort_by/limit/emit), the
// `investigate` keyword, `source <evidence>` references, `select ...
// as ...` aliases, and list literals. Each grammar rule maps to one
// parse_* method; ParseError carries line/col like LexError.

#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "jocky/ast/ast.hpp"
#include "jocky/lexer/lexer.hpp"

namespace jocky {

// Thrown on any syntax failure. Carries 1-based line/col of the
// offending token. Mirrors LexError's style (see lexer.hpp).
class ParseError : public std::runtime_error {
public:
    int line;
    int col;
    ParseError(int line, int col, const std::string& message)
        : std::runtime_error(message), line(line), col(col) {}
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens) : toks_(std::move(tokens)) {}

    Program parse_program() {
        Program prog;
        while (!check(TokenKind::EndOfFile)) {
            if (match_keyword("case")) {
                prog.cases.push_back(parse_case_decl());
            } else if (match_keyword("evidence")) {
                prog.evidence.push_back(parse_evidence_decl());
            } else if (match_keyword("rule")) {
                prog.rules.push_back(parse_rule_decl());
            } else if (match_keyword("investigate")) {
                prog.investigations.push_back(parse_investigation_decl());
            } else {
                fail("expected case, evidence, rule, or investigate "
                     "declaration");
            }
        }
        return prog;
    }

private:
    std::vector<Token> toks_;
    std::size_t pos_ = 0;

    const Token& peek() const { return toks_[pos_]; }

    const Token& previous() const { return toks_[pos_ - 1]; }

    bool check(TokenKind kind) const { return peek().kind == kind; }

    bool check_keyword(const std::string& word) const {
        return peek().kind == TokenKind::Keyword && peek().lexeme == word;
    }

    bool check_symbol(const std::string& sym) const {
        return peek().kind == TokenKind::Symbol && peek().lexeme == sym;
    }

    [[noreturn]] void fail(const std::string& message) {
        throw ParseError(peek().line, peek().col, message);
    }

    [[noreturn]] void fail_at(const Token& tok, const std::string& message) {
        throw ParseError(tok.line, tok.col, message);
    }

    const Token& advance() {
        if (!check(TokenKind::EndOfFile)) {
            ++pos_;
        }
        return previous();
    }

    bool match_keyword(const std::string& word) {
        if (check_keyword(word)) {
            advance();
            return true;
        }
        return false;
    }

    bool match_symbol(const std::string& sym) {
        if (check_symbol(sym)) {
            advance();
            return true;
        }
        return false;
    }

    std::string expect_ident(const std::string& what) {
        if (peek().kind != TokenKind::Identifier) {
            fail("expected " + what + ", found '" + peek().lexeme + "'");
        }
        std::string name = peek().lexeme;
        advance();
        return name;
    }

    void expect_keyword(const std::string& word) {
        if (!match_keyword(word)) {
            fail("expected keyword '" + word + "', found '" + peek().lexeme +
                 "'");
        }
    }

    void expect_symbol(const std::string& sym) {
        if (!match_symbol(sym)) {
            fail("expected '" + sym + "', found '" + peek().lexeme + "'");
        }
    }

    std::string expect_string(const std::string& what) {
        if (peek().kind != TokenKind::StringLit) {
            fail("expected " + what + ", found '" + peek().lexeme + "'");
        }
        std::string value = peek().lexeme;
        advance();
        return value;
    }

    // case_decl := "case" ident "{" { case_field } "}"
    CaseDecl parse_case_decl() {
        CaseDecl decl;
        decl.name = expect_ident("case name");
        expect_symbol("{");
        while (!check_symbol("}")) {
            if (check(TokenKind::EndOfFile)) {
                fail("unterminated case block");
            }
            std::string field = expect_ident("case field");
            if (field != "allowed_capabilities") {
                fail_at(previous(), "unknown case field '" + field + "'");
            }
            expect_symbol(":");
            expect_symbol("[");
            if (!check_symbol("]")) {
                decl.capabilities.push_back(expect_string("capability"));
                while (match_symbol(",")) {
                    decl.capabilities.push_back(expect_string("capability"));
                }
            }
            expect_symbol("]");
            expect_symbol(";");
        }
        expect_symbol("}");
        return decl;
    }

    // evidence_decl := "evidence" ident ":" adapter "(" string ")" ";"
    EvidenceDecl parse_evidence_decl() {
        EvidenceDecl decl;
        decl.name = expect_ident("evidence name");
        expect_symbol(":");
        if (peek().kind != TokenKind::Keyword ||
            (peek().lexeme != "pcap" && peek().lexeme != "eventlog" &&
             peek().lexeme != "directory")) {
            fail("expected adapter (pcap, eventlog, directory), found '" +
                 peek().lexeme + "'");
        }
        decl.adapter = peek().lexeme;
        advance();
        expect_symbol("(");
        decl.path = expect_string("evidence path");
        expect_symbol(")");
        expect_symbol(";");
        return decl;
    }

    // type := ident ["<" type {"," type} ">"]
    TypeRef parse_type() {
        TypeRef type;
        type.name = expect_ident("type name");
        if (match_symbol("<")) {
            type.args.push_back(parse_type());
            while (match_symbol(",")) {
                type.args.push_back(parse_type());
            }
            expect_symbol(">");
        }
        return type;
    }

    // block := "{" { stmt } "}"
    std::vector<PipelineStmt> parse_block() {
        std::vector<PipelineStmt> body;
        expect_symbol("{");
        while (!check_symbol("}")) {
            if (check(TokenKind::EndOfFile)) {
                fail("unterminated block");
            }
            body.push_back(parse_stmt());
        }
        expect_symbol("}");
        return body;
    }

    // rule_decl := "rule" ident "(" [params] ")" "->" type block
    RuleDecl parse_rule_decl() {
        RuleDecl decl;
        decl.name = expect_ident("rule name");
        expect_symbol("(");
        if (!check_symbol(")")) {
            decl.params.push_back(parse_param());
            while (match_symbol(",")) {
                decl.params.push_back(parse_param());
            }
        }
        expect_symbol(")");
        expect_symbol("->");
        decl.returns = parse_type();
        decl.body = parse_block();
        return decl;
    }

    Param parse_param() {
        Param param;
        param.name = expect_ident("parameter name");
        expect_symbol(":");
        param.type = parse_type();
        return param;
    }

    // investigation_decl := "investigate" ident block
    InvestigationDecl parse_investigation_decl() {
        InvestigationDecl decl;
        decl.name = expect_ident("investigation name");
        decl.body = parse_block();
        return decl;
    }

    // stmt := ["let" ident "="] pipeline_expr ";"
    PipelineStmt parse_stmt() {
        PipelineStmt stmt;
        if (match_keyword("let")) {
            stmt.binding = expect_ident("binding name");
            stmt.has_binding = true;
            expect_symbol("=");
        }
        stmt.expr = parse_pipeline_expr();
        expect_symbol(";");
        return stmt;
    }

    // pipeline_expr := expr { "|" pipeline_op }
    PipelineExpr parse_pipeline_expr() {
        PipelineExpr expr;
        expr.head = parse_expr_value();
        while (match_symbol("|")) {
            PipelineStep step;
            step.op = parse_pipeline_op();
            expr.steps.push_back(std::move(step));
        }
        return expr;
    }

    // expr := source_call | call_expr | field | literal | list
    // source_call := "source" ident  ("source" is a contextual keyword:
    // it lexes as Identifier and is only special in head position
    // followed by another identifier.)
    std::unique_ptr<ExprValue> parse_expr_value() {
        auto value = std::make_unique<ExprValue>();
        if (check_keyword("call")) {
            advance();
            value->kind = ExprValue::Kind::Call;
            value->call = parse_call();
            return value;
        }
        if (check_keyword("correlate")) {
            advance();
            value->kind = ExprValue::Kind::Correlate;
            value->correlate = parse_correlate_spec();
            return value;
        }
        if (peek().kind == TokenKind::Identifier && peek().lexeme == "source" &&
            pos_ + 1 < toks_.size() &&
            toks_[pos_ + 1].kind == TokenKind::Identifier) {
            advance();
            value->kind = ExprValue::Kind::Source;
            value->source = expect_ident("evidence name");
            return value;
        }
        if (peek().kind == TokenKind::Identifier) {
            value->kind = ExprValue::Kind::Field;
            value->field = parse_field();
            return value;
        }
        if (peek().kind == TokenKind::StringLit) {
            value->kind = ExprValue::Kind::String;
            value->str = peek().lexeme;
            advance();
            return value;
        }
        if (peek().kind == TokenKind::IntLit) {
            value->kind = ExprValue::Kind::Int;
            value->integer = std::stoll(peek().lexeme);
            advance();
            return value;
        }
        if (peek().kind == TokenKind::FloatLit) {
            value->kind = ExprValue::Kind::Float;
            value->floating = std::stod(peek().lexeme);
            advance();
            return value;
        }
        if (peek().kind == TokenKind::BoolLit) {
            value->kind = ExprValue::Kind::Bool;
            value->boolean = (peek().lexeme == "true");
            advance();
            return value;
        }
        if (check_symbol("[")) {
            advance();
            value->kind = ExprValue::Kind::List;
            if (!check_symbol("]")) {
                value->list.push_back(parse_expr_value());
                while (match_symbol(",")) {
                    value->list.push_back(parse_expr_value());
                }
            }
            expect_symbol("]");
            return value;
        }
        fail("expected expression, found '" + peek().lexeme + "'");
    }

    // call := "call" dotted_name "(" [named_arg {"," named_arg}] ")"
    std::unique_ptr<CallExpr> parse_call() {
        auto call = std::make_unique<CallExpr>();
        call->function.push_back(expect_ident("function name"));
        while (match_symbol(".")) {
            call->function.push_back(expect_ident("function name segment"));
        }
        expect_symbol("(");
        if (!check_symbol(")")) {
            call->args.push_back(parse_named_arg());
            while (match_symbol(",")) {
                call->args.push_back(parse_named_arg());
            }
        }
        expect_symbol(")");
        return call;
    }

    CallExpr::NamedArg parse_named_arg() {
        CallExpr::NamedArg arg;
        arg.name = expect_ident("argument name");
        expect_symbol(":");
        arg.value = parse_expr_value();
        return arg;
    }

    // field := ident {"." ident}
    FieldRef parse_field() {
        FieldRef field;
        field.path.push_back(expect_ident("field name"));
        while (match_symbol(".")) {
            field.path.push_back(expect_ident("field name segment"));
        }
        return field;
    }

    std::vector<FieldRef> parse_field_list() {
        std::vector<FieldRef> fields;
        fields.push_back(parse_field());
        while (match_symbol(",")) {
            fields.push_back(parse_field());
        }
        return fields;
    }

    // predicate := or_expr ; or_expr := and_expr {"or" and_expr}
    std::unique_ptr<Predicate> parse_predicate() { return parse_or(); }

    std::unique_ptr<Predicate> parse_or() {
        auto left = parse_and();
        while (match_keyword("or")) {
            auto node = std::make_unique<Predicate>();
            node->kind = Predicate::Kind::Or;
            node->operands.push_back(std::move(left));
            node->operands.push_back(parse_and());
            left = std::move(node);
        }
        return left;
    }

    std::unique_ptr<Predicate> parse_and() {
        auto left = parse_unary();
        while (match_keyword("and")) {
            auto node = std::make_unique<Predicate>();
            node->kind = Predicate::Kind::And;
            node->operands.push_back(std::move(left));
            node->operands.push_back(parse_unary());
            left = std::move(node);
        }
        return left;
    }

    std::unique_ptr<Predicate> parse_unary() {
        if (match_keyword("not")) {
            auto node = std::make_unique<Predicate>();
            node->kind = Predicate::Kind::Not;
            node->inner = parse_unary();
            return node;
        }
        return parse_comparison();
    }

    // comparison := operand [comp_op operand]
    std::unique_ptr<Predicate> parse_comparison() {
        auto left = parse_operand();
        std::string op;
        if (peek().kind == TokenKind::Symbol &&
            (peek().lexeme == "==" || peek().lexeme == "!=" ||
             peek().lexeme == "<" || peek().lexeme == "<=" ||
             peek().lexeme == ">" || peek().lexeme == ">=")) {
            op = peek().lexeme;
            advance();
        } else if (check_keyword("in") || check_keyword("contains") ||
                   check_keyword("contains_any")) {
            op = peek().lexeme;
            advance();
        }
        auto node = std::make_unique<Predicate>();
        if (op.empty()) {
            node->kind = Predicate::Kind::Atom;
            node->atom = std::move(left);
        } else {
            node->kind = Predicate::Kind::Compare;
            node->op = op;
            node->left = std::move(left);
            node->right = parse_operand();
        }
        return node;
    }

    std::unique_ptr<ExprValue> parse_operand() {
        if (check_keyword("call")) {
            advance();
            auto value = std::make_unique<ExprValue>();
            value->kind = ExprValue::Kind::Call;
            value->call = parse_call();
            return value;
        }
        if (peek().kind == TokenKind::Identifier) {
            auto value = std::make_unique<ExprValue>();
            value->kind = ExprValue::Kind::Field;
            value->field = parse_field();
            return value;
        }
        if (peek().kind == TokenKind::StringLit ||
            peek().kind == TokenKind::IntLit ||
            peek().kind == TokenKind::FloatLit ||
            peek().kind == TokenKind::BoolLit || check_symbol("[")) {
            return parse_expr_value();
        }
        fail("expected operand, found '" + peek().lexeme + "'");
    }

    // correlate_spec := "(" ident "," ident ")" "within" int unit "on"
    // field_list — shared by the expression-head and operator positions.
    CorrelateSpec parse_correlate_spec() {
        CorrelateSpec spec;
        expect_symbol("(");
        spec.left = expect_ident("left table");
        expect_symbol(",");
        spec.right = expect_ident("right table");
        expect_symbol(")");
        expect_keyword("within");
        if (peek().kind != TokenKind::IntLit) {
            fail("expected integer window size, found '" + peek().lexeme +
                 "'");
        }
        spec.within_value = std::stoll(peek().lexeme);
        advance();
        if (peek().kind != TokenKind::Identifier) {
            fail("expected duration unit (ms, s, m, h, d), found '" +
                 peek().lexeme + "'");
        }
        spec.within_unit = peek().lexeme;
        if (spec.within_unit != "ms" && spec.within_unit != "s" &&
            spec.within_unit != "m" && spec.within_unit != "h" &&
            spec.within_unit != "d") {
            fail_at(peek(), "unknown duration unit '" + spec.within_unit +
                                "' (expected ms, s, m, h, or d)");
        }
        advance();
        expect_keyword("on");
        spec.on_fields = parse_field_list();
        return spec;
    }

    // pipeline_op dispatch on leading keyword.
    PipelineOp parse_pipeline_op() {
        if (peek().kind != TokenKind::Keyword) {
            fail("expected pipeline operator, found '" + peek().lexeme + "'");
        }
        const std::string word = peek().lexeme;
        if (word == "filter" || word == "where" || word == "having") {
            advance();
            PipelineOp op;
            op.kind = (word == "filter")  ? PipelineOp::Kind::Filter
                      : (word == "where") ? PipelineOp::Kind::Where
                                          : PipelineOp::Kind::Having;
            op.predicate = parse_predicate();
            return op;
        }
        if (word == "select") {
            advance();
            PipelineOp op;
            op.kind = PipelineOp::Kind::Select;
            op.select_items.push_back(parse_select_item());
            while (match_symbol(",")) {
                op.select_items.push_back(parse_select_item());
            }
            return op;
        }
        if (word == "correlate") {
            advance();
            PipelineOp op;
            op.kind = PipelineOp::Kind::Correlate;
            op.correlate = parse_correlate_spec();
            return op;
        }
        if (word == "write") {
            advance();
            PipelineOp op;
            op.kind = PipelineOp::Kind::Write;
            if (check_keyword("report")) {
                advance();
                op.write_is_report = true;
                expect_symbol("(");
                if (peek().kind == TokenKind::StringLit) {
                    op.write_path = peek().lexeme;
                    advance();
                } else if (peek().kind == TokenKind::Identifier) {
                    op.write_path = peek().lexeme;
                    advance();
                } else {
                    fail("expected report path, found '" + peek().lexeme +
                         "'");
                }
                expect_symbol(")");
            } else if (peek().kind == TokenKind::StringLit) {
                op.write_path = peek().lexeme;
                advance();
            } else {
                fail("expected output path or report(...), found '" +
                     peek().lexeme + "'");
            }
            return op;
        }
        if (word == "group_by") {
            advance();
            PipelineOp op;
            op.kind = PipelineOp::Kind::GroupBy;
            op.group_fields = parse_field_list();
            return op;
        }
        if (word == "sort_by") {
            advance();
            PipelineOp op;
            op.kind = PipelineOp::Kind::SortBy;
            op.sort_fields = parse_field_list();
            if (peek().kind == TokenKind::Identifier &&
                (peek().lexeme == "asc" || peek().lexeme == "desc")) {
                op.sort_dir = peek().lexeme;
                advance();
            }
            return op;
        }
        if (word == "limit") {
            advance();
            PipelineOp op;
            op.kind = PipelineOp::Kind::Limit;
            if (peek().kind != TokenKind::IntLit) {
                fail("expected integer limit, found '" + peek().lexeme + "'");
            }
            op.limit = std::stoll(peek().lexeme);
            advance();
            return op;
        }
        if (word == "emit") {
            advance();
            PipelineOp op;
            op.kind = PipelineOp::Kind::Emit;
            if (peek().kind == TokenKind::StringLit) {
                op.emit_target = peek().lexeme;
                op.emit_is_string = true;
                advance();
            } else if (peek().kind == TokenKind::Identifier ||
                       peek().kind == TokenKind::Keyword) {
                op.emit_target = peek().lexeme;
                advance();
            } else {
                fail("expected emit target, found '" + peek().lexeme + "'");
            }
            return op;
        }
        fail("expected pipeline operator, found keyword '" + word + "'");
    }

    // select_item := field ["as" ident]
    SelectItem parse_select_item() {
        SelectItem item;
        item.field = parse_field();
        if (match_keyword("as")) {
            item.alias = expect_ident("alias");
            item.has_alias = true;
        }
        return item;
    }
};

}  // namespace jocky
