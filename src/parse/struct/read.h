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
        std::string id;
    };

    struct PushCommand : Command {
        PushCommand()
            : Command(CommandKind::push) {}
        std::shared_ptr<Expr> numpop;
        std::string id;
    };

    struct Read : Element {
        Read()
            : Element(ElementType::read) {}
        std::string name;
        std::vector<std::string> args;
        std::vector<std::shared_ptr<Command>> cmds;
    };
}  // namespace binred
