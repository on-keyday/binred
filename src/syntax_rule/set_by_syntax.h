/*
    binred - binary I/O code generator
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once
#include "../parse/struct/element.h"
#include "../syntax/syntax.h"

namespace binred {
    using SyntaxCb = Callback<syntax::MatchingErr, const syntax::MatchingContext&>;

    template <class Stmt>
    struct InvokeProxy {
        Stmt* stmt;

        InvokeProxy() {
            stmt = new Stmt();
        }

        bool operator()(const syntax::MatchingContext& ctx) {
            return (*stmt)(ctx);
        }

        ~InvokeProxy() {
            delete stmt;
        }
    };

    struct Stmt {
        bool ended = false;
        bool end_stmt() const {
            return ended;
        }
    };

    struct TreeBySyntax {
        std::shared_ptr<Expr> e;

        bool operator()(const syntax::MatchingContext& ctx) {
            if (ctx.is_under("EXPR")) {
                ExprKind kind = ExprKind::ref;
                auto set_to = [&](auto& ptr) {
                    ptr = std::make_shared<Expr>();
                    ptr->kind = kind;
                    ptr->token = ctx.get_tokloc().lock();
                    ptr->v = ctx.get_token();
                };
                auto to_prim = [&]() {
                    if (!e) {
                        set_to(e);
                    }
                    else if (!e->right) {
                        set_to(e->right);
                    }
                    else {
                        ctx.set_errmsg("invalid tree structure");
                        return false;
                    }
                    return true;
                };
                if (ctx.is_type(syntax::MatchingType::keyword)) {
                    if (ctx.is_token("nil")) {
                        kind = ExprKind::nil;
                    }
                    else if (ctx.is_token("true") || ctx.is_token("false")) {
                        kind = ExprKind::boolean;
                    }
                    else {
                        ctx.set_errmsg("unexpected keyword " + ctx.get_token());
                        return false;
                    }
                    return to_prim();
                }
                else if (ctx.is_type(syntax::MatchingType::identifier)) {
                    return to_prim();
                }
                else if (ctx.is_type(syntax::MatchingType::integer) || ctx.is_type(syntax::MatchingType::number)) {
                    kind = ExprKind::number;
                    return to_prim();
                }
                else if (ctx.is_type(syntax::MatchingType::string)) {
                    kind = ExprKind::str;
                    return to_prim();
                }
                else if (ctx.is_type(syntax::MatchingType::symbol)) {
                    if (ctx.is_token("(") || ctx.is_token(")")) {
                        return true;
                    }
                    if (!e) {
                        set_to(e);
                    }
                    else if (!e->left) {
                        std::shared_ptr<Expr> tmp;
                        set_to(tmp);
                        tmp->left = e;
                        e = tmp;
                    }
                    else {
                        ctx.set_errmsg("unexpected keyword " + ctx.get_token());
                        return false;
                    }
                }
                return true;
            }
            ctx.set_errmsg("expected expr but not");
            return false;
        }
    };

    struct Stmts;

    struct IfStmt {
        bool keyword = false;
        syntax::MatchingStackInfo stack;
        SyntaxCb cb;
        std::shared_ptr<Expr> init;
        std::shared_ptr<Expr> cond;
        bool operator()(const syntax::MatchingContext& ctx) {
            if (!keyword) {
                if (ctx.is_current("IFSTMT") && ctx.is_type(syntax::MatchingType::keyword) && ctx.is_token("if")) {
                    keyword = true;
                    stack = ctx.get_stack();
                }
                else {
                    ctx.set_errmsg("expect if statement but not");
                    return false;
                }
                return true;
            }
            if (ctx.is_rollbacked(stack)) {
                ctx.set_errmsg("unexpected rollback. if statement expected.");
                return false;
            }
            if (ctx.is_current(stack)) {
                if (ctx.is_token(";")) {
                    auto tree = cb.get_rawfunc<TreeBySyntax>();
                    if (!tree) {
                        ctx.set_errmsg("syntax parser is broken");
                        return false;
                    }
                    init = std::move(tree->e);
                    return true;
                }
                else if (ctx.is_token("{")) {
                    auto tree = cb.get_rawfunc<TreeBySyntax>();
                    if (!tree) {
                        ctx.set_errmsg("syntax parser is broken");
                        return false;
                    }
                    cond = std::move(tree->e);
                    cb = InvokeProxy<Stmts>();
                }
                else if (ctx.is_token("}")) {
                    auto stmts = cb.get_rawfunc<InvokeProxy<Stmts>>();
                }
                else {
                    ctx.set_errmsg("unexpected symbol " + ctx.get_token() + ". expect ; or {");
                    return false;
                }
                return true;
            }
            else {
                if (!cb) {
                    cb = TreeBySyntax();
                }
                return cb(ctx);
            }
        }
    };

    struct Stmts {
        bool operator()(const syntax::MatchingContext& ctx) {
        }
    };
}  // namespace binred