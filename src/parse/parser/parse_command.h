/*license*/
#pragma once

#include "../struct/read.h"
#include "../struct/record.h"
#include "parse_tree.h"
#include "parser.h"
namespace binred {
    bool parse_pop(TokenReader& r, std::shared_ptr<Command>& cmd, Record& rec) {
        auto e = r.ReadorEOF();
        if (!e) {
            return false;
        }
        if (!e->is_(TokenKind::keyword) || !e->has_("pop")) {
            r.SetError(ErrorCode::expect_keyword, "pop");
            return false;
        }
        e = r.ConsumeReadorEOF();
        if (!e) {
            return false;
        }
        auto tmp = std::make_shared<PopCommand>();
        //auto tree = get_tree();
        tmp->numpop = binary(r, rec.get_tree(), rec);
        if (!tmp->numpop) {
            return false;
        }
        if (e->has_("$")) {
            r.Consume();
            std::string name;
            if (!read_idlist(r, name)) {
                return false;
            }
        }
    }

    bool parse_command(TokenReader& r, std::vector<std::shared_ptr<Command>>& cmds, Record& rec) {
        while (true) {
            auto e = r.ReadorEOF();
            if (!e) {
                return false;
            }
            std::shared_ptr<Command> cmd;
            if (e->is_(TokenKind::keyword)) {
                if (e->has_("pop")) {
                }
            }
        }
    }
}  // namespace binred