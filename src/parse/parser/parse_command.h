/*license*/
#pragma once

#include "../struct/read.h"
#include "../struct/record.h"
#include "parser.h"
namespace binred {
    bool read_pop(TokenReader& r, std::shared_ptr<Command>& cmd, Record& rec) {
    }

    bool read_command(TokenReader& r, std::vector<std::shared_ptr<Command>>& cmds, Record& rec) {
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