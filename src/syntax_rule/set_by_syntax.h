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
    using SyntaxCb = syntax::Callback<syntax::MatchingErr, const syntax::MatchingContext&>;

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
            }
            return true;
        }
    };

    struct IfStmt {
        bool keyword = false;
        syntax::MatchingStackInfo stack;
        std::shared_ptr<Expr> expr;
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
            if (stack.rootpos < ctx.get_tokpos()) {
                ctx.set_errmsg("unexpected rollback. if statement expected.");
                return false;
            }
        }
    };
}  // namespace binred