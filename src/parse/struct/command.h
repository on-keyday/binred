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
        bind,
        test,
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
        std::vector<std::shared_ptr<IfCondition>> ifs;
    };

    struct DefCommand : Command {
        DefCommand()
            : Command(CommandKind::def) {}
        std::shared_ptr<Param> param;
        std::shared_ptr<Value> default_value;
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

    struct TransferData {
        std::string cargoname;
        std::weak_ptr<Cargo> cargo;
    };

    struct TransferDirect : Command {
        TransferDirect()
            : Command(CommandKind::transfer_direct) {}
        TransferData data;
    };

    struct TransferIf : Command {
        TransferIf()
            : Command(CommandKind::transfer_if) {}
        TransferData data;
        std::shared_ptr<Expr> cond;
    };

    struct TransferSwitch : Command {
        TransferSwitch()
            : Command(CommandKind::transfer_switch) {}
        std::shared_ptr<Expr> cond;
        std::vector<std::pair<std::shared_ptr<Expr>, TransferData>> to;
        TransferData defaults;
    };

    struct BindCommand : Command {
        BindCommand()
            : Command(CommandKind::bind) {}
        std::shared_ptr<Expr> expr;
    };

    struct TestCommand : Command {
        TestCommand()
            : Command(CommandKind::test) {}
        std::shared_ptr<Expr> expr;
    };

    struct AssignCommand : Command {
        AssignCommand()
            : Command(CommandKind::assign) {}
        std::string target;
        std::shared_ptr<Expr> expr;
    };

}  // namespace binred
