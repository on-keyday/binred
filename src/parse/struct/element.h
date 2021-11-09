/*license*/
#pragma once

#include <tokenparser/tokendef.h>
#include <string>
namespace binred {
    using token = commonlib2::tokenparser::Token<std::string>;
    enum class ElementType {
        cargo,
        alias,
        read,
        write,
        macro,
        complex,
    };

    struct Element {
        ElementType type;
        Element(ElementType t)
            : type(t) {}
    };

    enum class ExprKind {
        number,
        macro,
        ref,
        op,
    };

    struct Expr {
        ExprKind kind;
        std::string v;
        std::shared_ptr<Expr> left;
        std::shared_ptr<Expr> right;
        std::shared_ptr<token> token;
    };

    struct Value {
        std::shared_ptr<Expr> expr;
        std::shared_ptr<token> token;
    };

    struct Condition {
        std::shared_ptr<Expr> expr;
        std::shared_ptr<token> token;
    };

}  // namespace binred