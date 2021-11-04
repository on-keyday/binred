/*license*/
#pragma once

#include "parser.h"
#include "alias.h"
#include "tree.h"
#include "parse_macro.h"

namespace binred {
    bool parse_alias(TokenReader& r, std::shared_ptr<Element>& elm, MacroExpander& mep) {
        auto e = r.ReadorEOF();
        if (!e) {
            return false;
        }
        if (!e->is_(TokenKind::keyword) && !e->has_("alias")) {
            r.SetError(ErrorCode::expect_keyword, "alias");
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
        std::string name = id->get_identifier();
        e = r.ConsumeReadorEOF();
        if (!e) {
            return false;
        }
        if (!e->has_("{")) {
            r.SetError(ErrorCode::expect_symbol, "{");
            return false;
        }
        auto tmp = std::make_shared<Alias>();
        tmp->name = name;
        r.Consume();
        auto tree = get_tree();
        while (true) {
            if (!mep.expand(r)) {
                return false;
            }
            auto e = r.ReadorEOF();
            if (!e) {
                return false;
            }
            if (e->has_("}")) {
                r.Consume();
                break;
            }
            if (id = e->identifier(); !id) {
                r.SetError(ErrorCode::expect_id);
                return false;
            }
            r.Consume();
            auto res = binary(r, tree, mep);
            if (!res) {
                return false;
            }
            auto v = std::make_shared<Value>();
            v->expr = res;
            if (!tmp->alias.insert({id->get_identifier(), v}).second) {
                r.SetError(ErrorCode::multiple_alias);
                return false;
            }
        }
        elm = tmp;
        return true;
    }
}  // namespace binred
