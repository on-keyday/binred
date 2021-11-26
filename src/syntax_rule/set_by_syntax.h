/*
    binred - binary I/O code generator
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once
#include "../parse/struct/element.h"
#include <syntax/syntax.h>

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

        void release() {
            stmt->release();
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
            ret->release();
            return std::shared_ptr<Base>(ret);
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
        funccall,
    };

#define HANDLE_ROLLBACK(MSG)        \
    if (ctx.is_rollbacked(stack)) { \
        ctx.set_errmsg(MSG);        \
        return false;               \
    }

    struct Stmt {
        constexpr Stmt(StmtType t)
            : type(t) {}
        StmtType type;
        SyntaxCb cb;
        commonlib2::syntax::MatchingStackInfo stack;
        bool firstcall = true;
        bool ended = false;
        bool end_stmt() const {
            return ended;
        }
    };

    template <class T>
    std::shared_ptr<Expr> get_expr(SyntaxCb& cb) {
        auto ptr = cb.template get_rawfunc<InvokeProxy<T>>();
        if (ptr) {
            return std::move(ptr->stmt->e);
        }
        return nullptr;
    }

    struct TreeBySyntax;

    struct FuncCall : Stmt {
        std::vector<std::shared_ptr<Expr>> args;
        FuncCall()
            : Stmt(StmtType::funccall) {}
        bool operator()(const syntax::MatchingContext& ctx) {
            if (firstcall) {
                stack = ctx.get_stack();
                firstcall = false;
                return true;
            }
            if (ctx.is_current(stack)) {
                if (ctx.is_token(",")) {
                    args.push_back(get_expr<TreeBySyntax>(cb));
                    return true;
                }
                else if (ctx.is_token(")")) {
                    auto e = get_expr<TreeBySyntax>(cb);
                    if (e) {
                        args.push_back(std::move(e));
                    }
                    ended = true;
                    return true;
                }
                ctx.set_errmsg("parser broken");
                return false;
            }
            else {
                if (!cb) {
                    cb = InvokeProxy<TreeBySyntax>();
                }
                return cb(ctx);
            }
        }
    };

    struct TreeBySyntax {
        SyntaxCb cb;
        std::shared_ptr<Expr> e;

        bool operator()(const syntax::MatchingContext& ctx) {
            if (cb) {
                auto ret = cb(ctx);
                auto ptr = cb.get_rawfunc<FuncCall>();
                if (!ptr) {
                    ctx.set_errmsg("syntax parser is broken");
                    return false;
                }
                if (ptr->end_stmt()) {
                    auto func = cb.move_from_rawfunc<FuncCall>(move_to_shared<FuncCall>());
                    if (!func) {
                        ctx.set_errmsg("syntax parser is broken");
                        return false;
                    }
                    cb = nullptr;
                    auto call = std::make_shared<CallExpr>();
                    call->args = func->args;
                    call->left = e;
                    e = call;
                }
                return true;
            }
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
                    if (ctx.is_current("FUNCCALL")) {
                        cb = FuncCall();
                        return cb(ctx);
                    }
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

    struct VarInitStmt : Stmt {
        VarInitStmt()
            : Stmt(StmtType::varinit) {}
        std::vector<std::string> varname;
        std::vector<std::shared_ptr<Expr>> init;
        bool operator()(const syntax::MatchingContext& ctx) {
            if (firstcall) {
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
                        firstcall = false;
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
                ctx.set_errmsg("unexpected rollback. expect var init");
                return false;
            }
            if (ctx.is_current(stack)) {
                auto tree = cb.get_rawfunc<TreeBySyntax>();
                if (!tree) {
                    ctx.set_errmsg("syntax parser is broken");
                    return false;
                }
                init.push_back(std::move(tree->e));
                if (ctx.is_type(syntax::MatchingType::eos)) {
                    ended = true;
                }
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
        std::shared_ptr<Expr> expr;
        bool operator()(const syntax::MatchingContext& ctx) {
            if (firstcall) {
                stack = ctx.get_stack();
                firstcall = false;
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

    struct IfStmt : Stmt {
        constexpr IfStmt()
            : Stmt(StmtType::if_) {}

        std::shared_ptr<Stmt> init;
        std::shared_ptr<Stmt> cond;
        std::shared_ptr<Stmts> stmts;
        bool operator()(const syntax::MatchingContext& ctx) {
            if (firstcall) {
                if (ctx.is_current("IFSTMT") && ctx.is_type(syntax::MatchingType::keyword) && ctx.is_token("if")) {
                    firstcall = false;
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
                    auto tree = cb.move_from_rawfunc<ExprStmt, VarInitStmt>(move_to_shared<Stmt>());
                    if (!tree) {
                        ctx.set_errmsg("parser is broken");
                        return false;
                    }
                    init = std::move(tree);
                    cb = ExprStmt();
                    return true;
                }
                else if (ctx.is_token("{")) {
                    auto tree = cb.move_from_rawfunc<ExprStmt, VarInitStmt>(move_to_shared<Stmt>());
                    if (!tree) {
                        ctx.set_errmsg("syntax parser is broken");
                        return false;
                    }
                    cond = std::move(tree);
                    cb = InvokeProxy<Stmts>();
                }
                else if (ctx.is_token("}")) {
                    auto tmp = cb.move_from_rawfunc<InvokeProxy<Stmts>>(move_from_InvokeProxy<Stmts>());
                    if (!tmp) {
                        ctx.set_errmsg("syntax parser is broken");
                        return false;
                    }
                    stmts = std::move(tmp);
                    cb = nullptr;
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
                    cb = VarInitStmt();
                }
                auto res = cb(ctx);
                auto ptr = cb.get_rawfunc<Stmt, ExprStmt, VarInitStmt>();
                if (ptr) {
                    if (ptr->end_stmt() && !res) {
                        cb = ExprStmt();
                        return cb(ctx);
                    }
                }
                return res;
            }
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

        void release() {
            cb = nullptr;
        }

        bool operator()(const syntax::MatchingContext& ctx) {
            if (ctx.is_type(syntax::MatchingType::eof)) {
                cb = nullptr;
                return true;
            }
            if (cb) {
                auto ret = cb(ctx);
                auto ptr = borrow_ptr(cb);
                if (!ptr) {
                    ctx.set_errmsg("invalid stmt structure");
                    return false;
                }
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
