/*license*/
#pragma once

#include "parser.h"
#include "../struct/io.h"
#include "../struct/record.h"
#include "parse_types.h"
#include "parse_command.h"

namespace binred {
    template <class Vec>
    bool parse_arglist(Vec& vec, TokenReader& r, Record& record) {
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
            vec.push_back(std::move(param));
        }
        return true;
    }

    template <class IOType, bool is_read>
    bool parse_io(TokenReader& r, std::shared_ptr<Element>& elm, Record& rec) {
        auto e = r.ReadorEOF();
        if (!e) {
            return false;
        }
        constexpr auto io = is_read ? "read" : "write";
        if (!e->is_(TokenKind::keyword) || !e->has_(io)) {
            r.SetError(ErrorCode::expect_keyword, io);
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
        auto redelm = std::make_shared<IOType>();
        redelm->name = e->to_string();
        e = r.ConsumeReadorEOF();
        if (!e) {
            return false;
        }
        if (e->has_("(")) {
            r.Consume();
            if (!parse_arglist(redelm->args, r, rec)) {
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
        r.Consume();
        while (true) {
            e = r.ReadorEOF();
            if (!e) {
                return false;
            }
            if (e->has_("}")) {
                r.Consume();
                break;
            }
            std::shared_ptr<Command> cmd;
            if (!parse_command(r, cmd, rec)) {
                return false;
            }
            redelm->cmds.push_back(std::move(cmd));
        }
        elm = redelm;
        return true;
    }
}  // namespace binred
