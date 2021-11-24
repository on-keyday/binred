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
        Stmt* stmt = nullptr;

        InvokeProxy() {
            stmt = new Stmt();
        }

        InvokeProxy(InvokeProxy&& v) noexcept {
            stmt = v.stmt;
            v.stmt = nullptr;
        }

        bool operator()(const syntax::MatchingContext& ctx) {
            return (*stmt)(ctx);
        }

        ~InvokeProxy() {
            delete stmt;
        }
    };

    template <class Base>
    auto move_from_InvokeProxy() {
        return [](auto&& v) -> std::shared_ptr<Base> {
            auto ret = v.stmt;
            v.stmt = nullptr;
            return std::shared_ptr<Base>(v.stmt);
        };
    };

    struct Stmts;
    struct IfStmt;
    struct FuncStmt;
    struct VarInitStmt;

    enum class StmtType {
        if_,
        stmts,
        expr,
        varinit,
    };

    struct Stmt {
        constexpr Stmt(StmtType t)
            : type(t) {}
        StmtType type;
        SyntaxCb cb;
        syntax::MatchingStackInfo stack;
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
                    kind = ExprKind::op;
                    if (!e) {
                        set_to(e);
                    }
                    else {
                        std::shared_ptr<Expr> tmp;
                        set_to(tmp);
                        tmp->left = e;
                        e = tmp;
                    }
                }
                return true;
            }
            ctx.set_errmsg("expected expr but not");
            return false;
        }
    };

    struct IfStmt : Stmt {
        constexpr IfStmt()
            : Stmt(StmtType::if_) {}
        bool keyword = false;
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
                    auto tmp = cb.move_from_rawfunc<InvokeProxy<Stmts>>(move_from_InvokeProxy<Stmts>());
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
        VarInitStmt()
            : Stmt(StmtType::varinit) {}
        std::vector<std::string> varname;
        std::shared_ptr<Expr> init;
        bool syminit = false;
        bool operator()(const syntax::MatchingContext& ctx) {
            if (!syminit) {
                if (ctx.is_rollbacked(stack)) {
                    ended = true;
                    return false;
                }
                else if (ctx.is_current("VARINIT")) {
                    if (ctx.is_token(":=")) {
                        if (varname.size() == 0) {
                            ctx.set_errmsg("syntax parser is broken at varinit");
                            return false;
                        }
                        syminit = true;
                        return true;
                    }
                    else if (ctx.is_token(",")) {
                        return true;
                    }
                    else if (ctx.is_type(syntax::MatchingType::identifier)) {
                        if (varname.size() == 0) {
                            stack = ctx.get_stack();
                        }
                        varname.push_back(ctx.get_token());
                        return true;
                    }
                    ctx.set_errmsg("syntax parser is broken at varinit");
                    return false;
                }
            }
            if (ctx.is_rollbacked(stack)) {
                ctx.set_errmsg("unexpected rollback. expect varinit");
                return false;
            }
            if (ctx.is_current(stack) && ctx.is_type(syntax::MatchingType::eos)) {
                auto tree = cb.get_rawfunc<TreeBySyntax>();
                if (!tree) {
                    ctx.set_errmsg("syntax parser is broken");
                    return false;
                }
                init = std::move(tree->e);
                ended = true;
                return true;
            }
            if (!cb) {
                cb = TreeBySyntax();
            }
            return cb(ctx);
        }
    };

    struct ExprStmt : Stmt {
        ExprStmt()
            : Stmt(StmtType::expr) {}
        bool first = false;
        std::shared_ptr<Expr> expr;
        bool operator()(const syntax::MatchingContext& ctx) {
            if (!first) {
                stack = ctx.get_stack();
                first = true;
                return true;
            }
            if (ctx.is_rollbacked(stack)) {
                ctx.set_errmsg("unexpexted rollback. expect expr");
                return false;
            }
            if (ctx.is_current(stack) && ctx.is_type(syntax::MatchingType::eos)) {
                auto tree = cb.get_rawfunc<TreeBySyntax>();
                if (!tree) {
                    ctx.set_errmsg("invalid syntax parser");
                    return false;
                }
                expr = std::move(tree->e);
                ended = true;
                return true;
            }
            if (!cb) {
                cb = TreeBySyntax();
            }
            return cb(ctx);
        }
    };

    struct Stmts : Stmt {
#define STMTLIST Stmt, Stmts, IfStmt, VarInitStmt, ExprStmt
        Stmts()
            : Stmt(StmtType::stmts) {}
        static Stmt* borrow_ptr(SyntaxCb& cb) {
            return cb.get_rawfunc<STMTLIST>();
        }

        static std::shared_ptr<Stmt> get_ptr(SyntaxCb& cb) {
            return cb.move_from_rawfunc<STMTLIST>(move_to_shared<Stmt>());
        }
#undef STMTLIST
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
                        if (ret) {
                            return ret;
                        }
                    }
                    else {
                        return ret;
                    }
                }
                else {
                    ctx.set_errmsg("invalid stmt structure");
                    return false;
                }
            }
            if (!cb) {
                if (ctx.is_current("IFSTMT")) {
                    cb = IfStmt();
                }
                else if (ctx.is_current("VARINIT")) {
                    cb = VarInitStmt();
                }
                else if (ctx.is_current("EXPRSTMT")) {
                    cb = ExprStmt();
                }
                else {
                    ctx.set_errmsg("unknown stmt " + ctx.current());
                    return false;
                }
                return cb(ctx);
            }
            return true;
        }
    };
}  // namespace binred
