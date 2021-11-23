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
                if (ctx.is_type("KEYWORD")) {
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
                else if (ctx.is_type("ID")) {
                    return to_prim();
                }
                else if (ctx.is_type("INTEGER") || ctx.is_type("NUMBER")) {
                    kind = ExprKind::number;
                    return to_prim();
                }
                else if (ctx.is_type("STRING")) {
                    kind = ExprKind::str;
                    return to_prim();
                }
                else if (ctx.is_type("SYMBOL")) {
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
        bool operator()(const syntax::MatchingContext& ctx) {
            if (!keyword) {
                if (ctx.is_current("IFSTMT") && ctx.is_type("KEYWORD") && ctx.is_token("if")) {
                    keyword = true;
                }
                else {
                    ctx.set_errmsg("expect if statement but not");
                    return false;
                }
                return true;
            }
        }
    };
}  // namespace binred