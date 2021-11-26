/*
    binred - binary I/O code generator
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once

#include <vector>
#include <string>

#include <syntax/syntax.h>

namespace binred {
    namespace stmt {
        namespace cl2s = commonlib2::syntax;

        enum class ExprKind {
            number,
            id,
            op,
            call,
            nil,
            boolean,
            str,
        };

        struct Expr {
            ExprKind kind;
            std::string v;
            std::shared_ptr<Expr> left;
            std::shared_ptr<Expr> right;
            std::shared_ptr<cl2s::token_t> token;
        };

        struct CallExpr : Expr {
            CallExpr()
                : Expr{.kind = ExprKind::call} {}
            std::vector<std::shared_ptr<Expr>> args;
        };

        enum class StmtType {
            varinit,
            if_,
            expr,
        };

        struct Stmt {
            StmtType type;
            constexpr Stmt(StmtType t)
                : type(t) {}
        };

        struct VarInit : Stmt {
            VarInit()
                : Stmt(StmtType::varinit) {}
            std::vector<std::string> varname;
            std::vector<std::shared_ptr<Expr>> inits;
        };

        struct ExprStmt : Stmt {
            constexpr ExprStmt()
                : Stmt(StmtType::expr) {}
            std::shared_ptr<Expr> expr;
        };

        struct IfStmt : Stmt {
            constexpr IfStmt()
                : Stmt(StmtType::if_) {}
            std::shared_ptr<Stmt> init;
            std::shared_ptr<ExprStmt> cond;
            std::vector<std::shared_ptr<Stmt>> block;
        };
    }  // namespace stmt
}  // namespace binred
