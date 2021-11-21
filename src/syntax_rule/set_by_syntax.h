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

        void operator()(const syntax::MatchingContext& ctx) {
            if (ctx.is_under("EXPR")) {
                if (ctx.is_type("ID")) {
                    if (!e) {
                        e = std::make_shared<Expr>();
                        e->kind = ExprKind::ref;
                        e->token = ctx.get_pos().lock();
                    }
                    else if (!e->right) {
                        e->left = std::make_shared<Expr>();
                        e->left->token = ctx.get_pos().lock();
                    }
                }
            }
        }
    };
}  // namespace binred