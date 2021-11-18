/*
    binred - binary I/O code generator
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once
#include "parser.h"
#include "../struct/macro.h"
#include "../struct/record.h"
namespace binred {
    bool parse_macro(TokenReader& r, std::shared_ptr<Element>& elm, Record& mep) {
        auto e = r.ReadorEOF();
        if (!e) {
            return false;
        }
        auto start = e;
        if (!e->is_(TokenKind::keyword) || !e->has_("macro")) {
            r.SetError(ErrorCode::expect_keyword, "macro");
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
        auto id = e->identifier();
        auto tmp = std::make_shared<Macro>(std::move(start));
        tmp->name = id->get_identifier();
        while (true) {
            e = r.ConsumeReadorEOF();
            if (!e) {
                return false;
            }
            if (e->has_(":")) {
                r.Consume();
                break;
            }
            if (!e->is_(TokenKind::identifiers)) {
                r.SetError(ErrorCode::expect_id);
                return false;
            }
            tmp->args.push_back(e->to_string());
        }
        r.Read();
        while (!r.is_EOF()) {
            auto e = r.Get();
            r.Consume();
            if (!e) {
                break;
            }
            if (e->has_("\\")) {
                e = r.Get();
                if (!e) {
                    break;
                }
                r.Consume();
                if (auto l = e->line()) {
                    if (l->get_linecount() != 1) {
                        r.SetError(ErrorCode::unexpected_line);
                        return false;
                    }
                }
            }
            else if (e->is_(TokenKind::line)) {
                break;
            }
            tmp->expand += e->to_string();
        }
        if (!mep.mep.add(tmp->name, tmp)) {
            r.SetError(ErrorCode::multiple_macro);
            return false;
        }
        elm = tmp;
        return true;
    }
}  // namespace binred
