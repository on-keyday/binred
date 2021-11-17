/*
    binred - binary reader code generator
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once

#include "element.h"
#include <vector>
#include "param.h"

namespace binred {
    enum class CommandKind {
        pop,
        push,
        call,
        if_,
        def,
        assign,
        transfer_direct,
        transfer_if,
        transfer_switch,
        bind,
        test,
    };

    struct Command {
        CommandKind kind;
        std::shared_ptr<token_t> token;
        Command(std::shared_ptr<token_t>&& tok, CommandKind k)
            : kind(k), token(std::move(tok)) {}
    };

    struct IfCondition {
        std::shared_ptr<Expr> expr;
        std::vector<std::shared_ptr<Command>> cmds;
    };

    struct IfCommand : Command {
        IfCommand(std::shared_ptr<token_t>&& tok)
            : Command(std::move(tok), CommandKind::if_) {}
        std::vector<std::shared_ptr<IfCondition>> ifs;
    };

    struct DefCommand : Command {
        DefCommand(std::shared_ptr<token_t>&& tok)
            : Command(std::move(tok), CommandKind::def) {}
        std::shared_ptr<Param> param;
        std::shared_ptr<Value> default_value;
    };

    struct PopCommand : Command {
        PopCommand(std::shared_ptr<token_t>&& tok)
            : Command(std::move(tok), CommandKind::pop) {}
        std::shared_ptr<Expr> numpop;
        std::string refid;
    };

    struct PushCommand : Command {
        PushCommand(std::shared_ptr<token_t>&& tok)
            : Command(std::move(tok), CommandKind::push) {}
        std::shared_ptr<Expr> numpop;
        std::string refid;
    };

    struct CallCommand : Command {
        CallCommand(std::shared_ptr<token_t>&& tok)
            : Command(std::move(tok), CommandKind::call) {}
        std::shared_ptr<CallExpr> call;
    };

    struct TransferData {
        std::string cargoname;
        std::weak_ptr<Cargo> cargo;
    };

    struct TransferDirect : Command {
        TransferDirect(std::shared_ptr<token_t>&& tok)
            : Command(std::move(tok), CommandKind::transfer_direct) {}
        TransferData data;
    };

    struct TransferIf : Command {
        TransferIf(std::shared_ptr<token_t>&& tok)
            : Command(std::move(tok), CommandKind::transfer_if) {}
        TransferData data;
        std::shared_ptr<Expr> cond;
    };

    struct TransferSwitch : Command {
        TransferSwitch(std::shared_ptr<token_t>&& tok)
            : Command(std::move(tok), CommandKind::transfer_switch) {}
        std::shared_ptr<Expr> cond;
        std::vector<std::pair<std::shared_ptr<Expr>, TransferData>> to;
        TransferData defaults;
    };

    struct BindCommand : Command {
        BindCommand(std::shared_ptr<token_t>&& tok)
            : Command(std::move(tok), CommandKind::bind) {}
        std::shared_ptr<Expr> expr;
    };

    struct TestCommand : Command {
        TestCommand(std::shared_ptr<token_t>&& tok)
            : Command(std::move(tok), CommandKind::test) {}
        std::shared_ptr<Expr> expr;
    };

    struct AssignCommand : Command {
        AssignCommand(std::shared_ptr<token_t>&& tok)
            : Command(std::move(tok), CommandKind::assign) {}
        std::string target;
        std::shared_ptr<Expr> expr;
    };

}  // namespace binred
