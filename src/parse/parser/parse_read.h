/*license*/
#pragma once

#include "parser.h"
#include "../struct/read.h"
#include "../struct/record.h"
#include "parse_types.h"
#include "parse_command.h"

namespace binred {
    bool parse_arglist(TokenReader& r, std::shared_ptr<Read>& red, Record& record) {
        while (true) {
            auto e = r.ReadorEOF();
            if (!e) {
                return false;
            }
            if (e->has_(")")) {
                r.Consume();
                break;
            }
            if (!e->is_(TokenKind::identifiers)) {
                return false;
            }
            std::shared_ptr<Param> param;
            if (!parse_type(r, param, record)) {
                return false;
            }
            param->name = e->to_string();
        }
        return true;
    }

    bool parse_read(TokenReader& r, std::shared_ptr<Element>& elm, Record& rec) {
        auto e = r.ReadorEOF();
        if (!e) {
            return false;
        }
        if (!e->is_(TokenKind::keyword) || !e->has_("read")) {
            r.SetError(ErrorCode::expect_keyword, "read");
            return false;
        }
        e = r.ConsumeReadorEOF();
        if (!e) {
            return false;
        }
        if (!e->is_(TokenKind::identifiers)) {
            r.SetError(ErrorCode::expect_id);
            return false;
        }
        auto redelm = std::make_shared<Read>();
        redelm->name = e->to_string();
        e = r.ConsumeReadorEOF();
        if (!e) {
            return false;
        }
        if (e->has_("(")) {
            r.Consume();
            if (!parse_arglist(r, redelm, rec)) {
                return false;
            }
            e = r.ReadorEOF();
            if (!e) {
                return false;
            }
        }
        if (!e->has_("{")) {
            r.SetError(ErrorCode::expect_symbol, "{");
            return false;
        }
        return true;
    }
}  // namespace binred