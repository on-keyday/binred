/*license*/
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
    };

    struct Command {
        CommandKind kind;
        constexpr Command(CommandKind k)
            : kind(k) {}
    };

    struct IfCondition {
        std::shared_ptr<Expr> expr;
        std::vector<std::shared_ptr<Command>> cmds;
    };

    struct IfCommand : Command {
        IfCommand()
            : Command(CommandKind::if_) {}
        std::vector<IfCondition> ifs;
    };

    struct DefCommand : Command {
        DefCommand()
            : Command(CommandKind::def) {}
        std::shared_ptr<Param> param;
    };

    struct PopCommand : Command {
        PopCommand()
            : Command(CommandKind::pop) {}
        std::shared_ptr<Expr> numpop;
        std::string refid;
    };

    struct PushCommand : Command {
        PushCommand()
            : Command(CommandKind::push) {}
        std::shared_ptr<Expr> numpop;
        std::string refid;
    };

    struct CallCommand : Command {
        CallCommand()
            : Command(CommandKind::call) {}
        std::shared_ptr<CallExpr> call;
    };

    struct TransferDirect : Command {
        TransferDirect()
            : Command(CommandKind::transfer_direct) {}
        std::string cargoname;
    };

    struct TransferIf : Command {
        TransferIf()
            : Command(CommandKind::transfer_if) {}
        std::string cargoname;
        std::shared_ptr<Expr> cond;
    };

    struct TransferSwitch : Command {
        TransferSwitch()
            : Command(CommandKind::transfer_switch) {}
        std::shared_ptr<Expr> cond;
        std::vector<std::pair<std::shared_ptr<Expr>, std::string>> to;
        std::string defaults;
    };

}  // namespace binred