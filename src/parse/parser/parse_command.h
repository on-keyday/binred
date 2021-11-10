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

    bool parse_call(TokenReader& r, std::shared_ptr<Command>& cmd, Record& rec) {
        auto e = r.ReadorEOF();
        if (!e) {
            return false;
        }
        auto e = r.ReadorEOF();
        if (!e) {
            return false;
        }
        if (!e->is_(TokenKind::keyword) || !e->has_("call")) {
            r.SetError(ErrorCode::expect_keyword, "call");
            return false;
        }
        r.Consume();
        auto tmp = std::make_shared<CallCommand>();
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
                }
            }
        }
    }
}  // namespace binred