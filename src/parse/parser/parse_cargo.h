/*license*/
#pragma once
#include "parser.h"
#include "../struct/cargo.h"
#include "parse_tree.h"
#include <set>
#include "../struct/record.h"
#include "parse_types.h"

namespace binred {

    bool parse_baseinfo(TokenReader& r, BaseInfo& info, Record& mep) {
        EXPAND_MACRO(mep)
        auto e = r.ReadorEOF();
        if (!e) {
            return false;
        }
        if (!e->is_(TokenKind::identifiers)) {
            r.SetError(ErrorCode::expect_id);
            return false;
        }
        info.selfname = e->to_string();
        EXPAND_MACRO(mep)
        e = r.ConsumeReadorEOF();
        if (!e) {
            return false;
        }
        if (!e->is_(TokenKind::identifiers)) {
            r.SetError(ErrorCode::expect_id);
            return false;
        }
        info.basename = e->to_string();
        r.Consume();
        return true;
    }

    bool parse_cargo(TokenReader& r, std::shared_ptr<Element>& elm, Record& mep) {
        auto e = r.ReadorEOF();
        if (!e) {
            return false;
        }
        if (!e->is_(TokenKind::keyword) || !e->has_("cargo")) {
            r.SetError(ErrorCode::expect_keyword, "cargo");
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
        EXPAND_MACRO(mep)
        e = r.ConsumeReadorEOF();
        if (!e) {
            return false;
        }
        BaseInfo base;
        if (e->has_(":")) {
            r.Consume();
            if (!parse_baseinfo(r, base, mep)) {
                return false;
            }
            e = r.ReadorEOF();
            if (!e) {
                return false;
            }
        }
        EXPAND_MACRO(mep)
        if (!e->has_("{")) {
            r.SetError(ErrorCode::expect_symbol, "{");
            return false;
        }
        auto tmp = std::make_shared<Cargo>();
        tmp->name = name;
        tmp->base = std::move(base);
        r.Consume();
        std::set<std::string> already_set;
        if (base.selfname.size()) {
            already_set.emplace(base.selfname);
        }
        while (true) {
            EXPAND_MACRO(mep)
            e = r.ReadorEOF();
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
            std::shared_ptr<Param> param;
            if (!parse_type(r, param, mep)) {
                return false;
            }
            param->name = id->get_identifier();
            tmp->params.push_back(param);
            tmp->expanded = tmp->expanded || param->expand;
            if (!already_set.insert(param->name).second) {
                r.SetError(ErrorCode::multiple_variable);
                return false;
            }
        }
        if (!mep.add_cargo(name, tmp)) {
            r.SetError(ErrorCode::multiple_cargo);
            return false;
        }
        elm = tmp;
        return true;
    }
}  // namespace binred
