/*license*/
#pragma once

#include "parser.h"
#include "../struct/alias.h"
#include "parse_tree.h"
#include "parse_macro.h"
#include "../struct/record.h"
#include "parse_types.h"

namespace binred {

    bool parse_alias(TokenReader& r, std::shared_ptr<Element>& elm, Record& mep) {
        auto e = r.ReadorEOF();
        if (!e) {
            return false;
        }
        auto start = e;
        if (!e->is_(TokenKind::keyword) || !e->has_("alias")) {
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
        if (e->has_("=")) {
            r.Consume();
            auto tmp = std::make_shared<TypeAlias>(std::move(start));
            if (!parse_type(r, tmp->type, mep)) {
                return false;
            }
            tmp->type->name = name;
            if (!mep.add_types(name, tmp)) {
                r.SetError(ErrorCode::multiple_alias);
                return false;
            }
            elm = tmp;
            return true;
        }
        if (!e->has_("{")) {
            r.SetError(ErrorCode::expect_symbol, "{");
            return false;
        }
        auto tmp = std::make_shared<NumberAlias>(std::move(start));
        tmp->name = name;
        r.Consume();
        auto& tree = mep.get_tree();
        while (true) {
            EXPAND_MACRO(mep)
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
        if (!mep.add_alias(name, tmp)) {
            r.SetError(ErrorCode::multiple_alias);
            return false;
        }
        elm = tmp;
        return true;
    }
}  // namespace binred
