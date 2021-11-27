/*
    binred - binary I/O code generator
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once

#include "stmt.h"
#include <callback.h>

namespace binred {
    namespace stmt {
        using SyntaxCb = commonlib2::Callback<cl2s::MatchingErr, const cl2s::MatchingContext&, bool&>;
        struct ParserCallback {
            bool is_set = false;
            bool result = false;
            bool change_cb = false;
            cl2s::MatchingStackInfo stack;
            const cl2s::MatchingContext* ctx = nullptr;
            SyntaxCb cb;

            //return true if first time to call
            bool set_ctx(const cl2s::MatchingContext& ctx) {
                this->ctx = &ctx;
                result = false;
                change_cb = false;
                if (!is_set) {
                    stack = ctx.get_stack();
                    is_set = true;
                    return true;
                }
                return false;
            }

            bool is_roolbacked() const {
                return ctx->is_rollbacked(stack);
            }

            bool is_current() const {
                return ctx->is_current(stack);
            }

            bool callcb() {
                if (cb) {
                    result = (bool)cb(*ctx, change_cb);
                    return true;
                }
                return false;
            }

            bool is_cbsucceed() const {
                return result;
            }

            bool is_cbchange() const {
                return result && change_cb;
            }

            bool is_cbrollback() const {
                return !result && change_cb;
            }

            bool is_cbfailed() const {
                return !result && !change_cb;
            }

            bool broken() {
                ctx->set_errmsg("syntax parser is broken");
                return false;
            }

            bool rollback(bool& chcb) {
                chcb = true;
                return false;
            }

            bool finish(bool& chcb) {
                chcb = true;
                return true;
            }

            virtual std::shared_ptr<Stmt> get_as_stmt() {
                return nullptr;
            }
        };

        template <class ExprP>
        struct FuncCall : ParserCallback {
            std::shared_ptr<CallExpr> call;
            bool operator()(const cl2s::MatchingContext& ctx, bool& chcb) {
                if (set_ctx(&ctx)) {
                    call = std::make_shared<CallExpr>();
                }
                if (is_roolbacked()) {
                    return broken();
                }
                if (is_current()) {
                    if (ctx.is_token("(")) {
                        return true;
                    }
                    else if (ctx.is_token(",") || ctx.is_token(")")) {
                        auto p = cb.template get_rawfunc<ExprP>();
                        if (!p) {
                            return broken();
                        }
                        call->args.push_back(std::move(p->expr));
                        if (ctx.is_token(")")) {
                            return finish(chcb);
                        }
                        return true;
                    }
                    else {
                        return broken();
                    }
                }
                if (!cb) {
                    cb = ExprP();
                }
                callcb();
                return result;
            }
        };

        struct ExprParser : ParserCallback {
            std::shared_ptr<Expr> expr;
            bool operator()(const cl2s::MatchingContext& ctx, bool& chcb) {
                set_ctx(ctx);
                ExprKind kind = ExprKind::id;
                auto set_to = [&](auto& ptr) {
                    ptr = std::make_shared<Expr>();
                    ptr->v = ctx.get_token();
                    ptr->kind = kind;
                };
                auto set_prim = [&] {
                    if (!expr) {
                        set_to(expr);
                    }
                    else if (!expr->right) {
                        set_to(expr->right);
                    }
                    else {
                        return broken();
                    }
                    return true;
                };
                if (callcb()) {
                    if (is_cbchange()) {
                        auto p = cb.get_rawfunc<FuncCall<ExprParser>>();
                        if (!p) {
                            return broken();
                        }
                        p->call->left = expr;
                        expr = std::move(p->call);
                        return true;
                    }
                    else if (!is_cbsucceed()) {
                        return false;
                    }
                    return true;
                }
                if (ctx.is_current("FUNCCALL")) {
                    cb = FuncCall<ExprParser>();
                    callcb();
                    return result;
                }
                if (ctx.is_type(cl2s::MatchingType::symbol)) {
                    if (ctx.is_token("(") || ctx.is_token(")")) {
                        return true;
                    }
                    kind = ExprKind::op;
                    if (!expr) {
                        set_to(expr);
                    }
                    else {
                        std::shared_ptr<Expr> tmp;
                        set_to(tmp);
                        tmp->left = std::move(expr);
                        expr = std::move(tmp);
                    }
                }
                else if (ctx.is_type(cl2s::MatchingType::keyword)) {
                    if (ctx.is_token("nil")) {
                        kind = ExprKind::nil;
                    }
                    else if (ctx.is_token("true") || ctx.is_token("false")) {
                        kind = ExprKind::boolean;
                    }
                    else {
                        return broken();
                    }
                    return set_prim();
                }
                else if (ctx.is_type(cl2s::MatchingType::identifier)) {
                    kind = ExprKind::id;
                    return set_prim();
                }
                else if (ctx.is_type(cl2s::MatchingType::integer)) {
                    kind = ExprKind::integer;
                    return set_prim();
                }
                else if (ctx.is_type(cl2s::MatchingType::number)) {
                    kind = ExprKind::number;
                    return set_prim();
                }
                else if (ctx.is_type(cl2s::MatchingType::string)) {
                    kind = ExprKind::str;
                    return set_prim();
                }
                return broken();
            }
        };

        struct ExprStmtParser : ParserCallback {
            std::shared_ptr<ExprStmt> stmt;
            bool operator()(const cl2s::MatchingContext& ctx, bool& chcb) {
                if (set_ctx(ctx)) {
                    stmt = std::make_shared<ExprStmt>();
                    return true;
                }
                if (is_current()) {
                    if (ctx.is_type(cl2s::MatchingType::eos)) {
                        auto p = cb.move_from_rawfunc<ExprParser>(commonlib2::move_to_shared<ExprParser>());
                        if (!p) {
                            return broken();
                        }
                        stmt->expr = std::move(p->expr);
                        return true;
                    }
                    return broken();
                }
                if (!cb) {
                    cb = ExprParser();
                }
                callcb();
                return result;
            }
        };

        struct VarInitParser : ParserCallback {
            std::shared_ptr<VarInit> init;
            bool cantrollback = false;
            bool operator()(const cl2s::MatchingContext& ctx, bool& chcb) {
                if (set_ctx(ctx)) {
                    init = std::make_shared<VarInit>();
                }
                if (is_roolbacked()) {
                    if (cantrollback) {
                        return broken();
                    }
                    return rollback(chcb);
                }
                auto add_init = [&]() {
                    auto p = cb.get_rawfunc<ExprParser>();
                    if (!p) {
                        return broken();
                    }
                    init->inits.push_back(std::move(p->expr));
                    return true;
                };
                if (is_current()) {
                    if (ctx.is_type(cl2s::MatchingType::eos)) {
                        if (!add_init()) {
                            return false;
                        }
                        return finish(chcb);
                    }
                    else if (ctx.is_type(cl2s::MatchingType::identifier)) {
                        init->varname.push_back(ctx.get_token());
                        return true;
                    }
                    else if (ctx.is_token(":=")) {
                        cantrollback = true;
                        return true;
                    }
                    else if (ctx.is_token(",")) {
                        if (cantrollback) {
                            if (!add_init()) {
                                return false;
                            }
                        }
                        return true;
                    }
                    else {
                        return broken();
                    }
                }
                if (!cantrollback) {
                    return broken();
                }
                if (!cb) {
                    cb = ExprParser();
                }
                callcb();
                return result;
            }
            std::shared_ptr<Stmt> get_as_stmt() override {
                return std::move(init);
            }
        };

        template <class StmtsParser>
        struct IfStmtParser : ParserCallback {
            std::shared_ptr<IfStmt> stmt;
            bool operator()(const cl2s::MatchingContext& ctx, bool& chcb) {
                if (set_ctx(ctx)) {
                    stmt = std::shared_ptr<IfStmt>();
                    if (!ctx.is_token("if")) {
                        return broken();
                    }
                    return true;
                }
                auto get_to = [&](auto& to) {
                    auto parser = cb.move_from_rawfunc<ExprParser, VarInitParser>(commonlib2::move_to_shared<ParserCallback>());
                    auto p = parser->get_as_stmt();
                    if (!p) {
                        return broken();
                    }
                    to = std::move(p);
                    return true;
                };
                if (is_current()) {
                    if (ctx.is_token(";")) {
                        if (!get_to(stmt->init)) {
                            return false;
                        }
                        cb = ExprParser();
                        return true;
                    }
                    else if (ctx.is_token("{")) {
                        if (!get_to(stmt->cond)) {
                            return false;
                        }
                        cb = StmtsParser();
                        return true;
                    }
                    else if (ctx.is_token("}")) {
                        auto p = cb.move_from_rawfunc<StmtsParser>(commonlib2::move_to_shared<StmtsParser>());
                        if (!p) {
                            return broken();
                        }
                        stmt->block = std::move(p->stmt);
                        return finish();
                    }
                    return broken();
                }
                if (!cb) {
                    cb = VarInitParser();
                }
                callcb();
                if (is_cbrollback()) {
                    cb = ExprParser();
                    callcb();
                    return result;
                }
                return result;
            }
        };

        struct StmtsParser : ParserCallback {
            std::shared_ptr<Stmts> stmt;
            bool operator()(const cl2s::MatchingContext& ctx, bool& chcb) {
                if (set_ctx(ctx)) {
                    stmt = std::make_shared<Stmts>();
                }
                if (callcb()) {
                    if (is_cbchange()) {
                        auto p = cb.move_from_rawfunc<StmtsParser, IfStmtParser<Stmts>, VarInitParser, ExprStmtParser>(commonlib2::move_to_shared<ParserCallback>());
                        if (!p) {
                            return broken();
                        }
                        auto s = p->get_as_stmt();
                        if (!s) {
                            return broken();
                        }
                        stmt->block.push_back(std::move(s));
                        return true;
                    }
                    else if (is_cbrollback()) {
                        cb = nullptr;
                    }
                    else {
                        return false;
                    }
                }
                if (ctx.is_current("VARINIT")) {
                    cb = VarInitParser();
                }
                else if (ctx.is_current("IFSTMT")) {
                    cb = IfStmtParser<Stmts>();
                }
                else if (ctx.is_current("EXPRSTMT")) {
                    cb = ExprStmt();
                }
                callcb();
                return result;
            }
        };
    }  // namespace stmt
}  // namespace binred
