/*license*/
#pragma once

#include "../struct/read.h"
#include "../struct/record.h"
#include "parse_tree.h"
#include "parser.h"
namespace binred {

    template <class CmdType>
    bool parse_push_or_pop(TokenReader& r, std::shared_ptr<Command>& cmd, Record& rec, const char* poppush) {
        auto e = r.ReadorEOF();
        if (!e) {
            return false;
        }
        if (!e->is_(TokenKind::keyword) || !e->has_(poppush)) {
            r.SetError(ErrorCode::expect_keyword, poppush);
            return false;
        }
        e = r.ConsumeReadorEOF();
        if (!e) {
            return false;
        }
        auto tmp = std::make_shared<CmdType>();
        //auto tree = get_tree();
        tmp->numpop = binary(r, rec.get_tree(), rec);
        if (!tmp->numpop) {
            return false;
        }
        if (e->has_("$")) {
            r.Consume();
            if (!read_idname(r, tmp->refid)) {
                return false;
            }
        }
        cmd = tmp;
        return true;
    }

    std::shared_ptr<CallExpr> parse_callexpr(TokenReader& r, Record& rec) {
        auto e = r.ReadorEOF();
        if (!e) {
            return nullptr;
        }
        if (!e->is_(TokenKind::keyword) || !e->has_("call")) {
            r.SetError(ErrorCode::expect_keyword, "call");
            return nullptr;
        }
        r.Consume();
        auto expr = std::make_shared<CallExpr>();
        expr->token = e;
        std::string name;
        if (!read_idname(r, name)) {
            return nullptr;
        }
        expr->v = name;
        e = r.ReadorEOF();
        if (!e) {
            return nullptr;
        }
        if (!e->has_("(")) {
            r.SetError(ErrorCode::expect_symbol, "(");
            return nullptr;
        }
        r.Consume();
        while (true) {
            e = r.ReadorEOF();
            if (e->has_(")")) {
                r.Consume();
                break;
            }
            auto tmp = binary(r, rec.get_tree(), rec);
            if (!tmp) {
                return nullptr;
            }
            expr->args.push_back(tmp);
            e = r.ReadorEOF();
            if (!e) {
                return nullptr;
            }
            if (e->has_(",")) {
                r.Consume();
            }
        }
        return expr;
    }

    bool parse_callcommand(TokenReader& r, std::shared_ptr<Command>& cmd, Record& rec) {
        auto tmp = std::make_shared<CallCommand>();
        if (tmp->call = parse_callexpr(r, rec); !tmp->call) {
            return false;
        }
        return true;
    }

    bool parse_command(TokenReader& r, std::vector<std::shared_ptr<Command>>& cmds, Record& rec) {
        while (true) {
            auto e = r.ReadorEOF();
            if (!e) {
                return false;
            }
            std::shared_ptr<Command> cmd;
            if (e->is_(TokenKind::keyword)) {
                if (e->has_("push")) {
                    if (!parse_push_or_pop<PushCommand>(r, cmd, rec, "push")) {
                        return false;
                    }
                }
                else if (e->has_("pop")) {
                    if (!parse_push_or_pop<PopCommand>(r, cmd, rec, "pop")) {
                        return false;
                    }
                }
                else if (e->has_("call")) {
                    if (!parse_callcommand(r, cmd, rec)) {
                        return false;
                    }
                }
            }
        }
    }
}  // namespace binred
