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

    struct Stmts;
    struct IfStmt;
    struct FuncStmt;
    struct VarInitStmt;

    struct Stmt {
        SyntaxCb cb;
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

    struct IfStmt : Stmt {
        bool keyword = false;
        syntax::MatchingStackInfo stack;
        SyntaxCb cb;
        std::shared_ptr<Expr> init;
        std::shared_ptr<Expr> cond;
        std::shared_ptr<Stmts> stmts;
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
                    auto tmp = cb.move_from_rawfunc<InvokeProxy<Stmts>>([](auto&& v) {
                        auto ret = v.stmt;
                        v.stmt = nullptr;
                        return std::shared_ptr<Stmts>(v.stmt);
                    });
                    if (!tmp) {
                        ctx.set_errmsg("syntax parser is broken");
                        return false;
                    }
                    stmts = std::move(tmp);
                    ended = true;
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

    struct VarInitStmt : Stmt {
        std::vector<std::string> varname;
        bool syminit = false;
        bool operator()(const syntax::MatchingContext& ctx) {
            if (!syminit) {
                if (ctx.is_current("VARINIT")) {
                    if (ctx.is_token(":=")) {
                        if (varname.size() == 0) {
                            ctx.set_errmsg("syntax parser is broken at varinit");
                            return false;
                        }
                        syminit = true;
                        return true;
                    }
                }
            }
        }
    };

    struct Stmts : Stmt {
        static Stmt* borrow_ptr(SyntaxCb& cb) {
            return cb.get_rawfunc<Stmt, Stmts, IfStmt>();
        }

        static std::shared_ptr<Stmt> get_ptr(SyntaxCb& cb) {
            return cb.move_from_rawfunc<Stmt, Stmts, IfStmt>(move_to_shared<Stmt>());
        }

        std::vector<std::shared_ptr<Stmt>> stmts;

        bool operator()(const syntax::MatchingContext& ctx) {
            if (cb) {
                auto ret = cb(ctx);
                if (auto ptr = borrow_ptr(cb)) {
                    if (ptr->end_stmt()) {
                        if (ret) {
                            stmts.push_back(get_ptr(cb));
                        }
                        cb = nullptr;
                    }
                    return ret;
                }
                else {
                    ctx.set_errmsg("invalid stmt structure");
                    return false;
                }
                return ret;
            }
            else {
                if (ctx.is_current("IFSTMT")) {
                    cb = IfStmt();
                }
                else if (ctx.is_current("VARINIT")) {
                }
            }
            return true;
        }
    };
}  // namespace binred
