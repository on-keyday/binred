/*
    binred - binary reader code generator
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once

#include <tokenparser/tokendef.h>
#include <string>
namespace binred {
    using token_t = commonlib2::tokenparser::Token<std::string>;
    enum class ElementType {
        cargo,
        numalias,
        tyalias,
        read,
        write,
        macro,
        complex,
        func,
    };

    struct Element {
        ElementType type;
        std::shared_ptr<token_t> token;
        Element(std::shared_ptr<token_t>&& tok, ElementType t)
            : type(t), token(std::move(tok)) {}
    };

    enum class ExprKind {
        number,
        ref,
        op,
        call,
        nil,
        boolean,
    };

    struct Expr {
        ExprKind kind;
        std::string v;
        std::shared_ptr<Expr> left;
        std::shared_ptr<Expr> right;
        std::shared_ptr<token_t> token;
    };

    struct CallExpr : Expr {
        CallExpr()
            : Expr{.kind = ExprKind::call} {}
        std::vector<std::shared_ptr<Expr>> args;
    };

    struct Value {
        std::shared_ptr<Expr> expr;
        std::shared_ptr<token_t> token;
    };

    struct Condition {
        std::shared_ptr<Expr> expr;
        std::shared_ptr<token_t> token;
    };

}  // namespace binred
