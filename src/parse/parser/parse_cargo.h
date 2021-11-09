/*license*/
#pragma once
#include "parser.h"
#include "../struct/cargo.h"
#include "tree.h"
#include <set>
#include "../struct/record.h"

namespace binred {
    bool parse_length(TokenReader& r, std::shared_ptr<Length>& length, Record& mep) {
        if (!mep.expand(r)) {
            return false;
        }
        auto tree = get_tree();
        auto res = binary(r, tree, mep);
        if (!res) {
            return false;
        }
        auto tmp = std::make_shared<ExprLength>();
        tmp->expr = res;
        length = tmp;
        return true;
    }

    bool parse_condition(TokenReader& r, std::shared_ptr<Param>& param, Record& mep) {
        while (true) {
            if (!mep.expand(r)) {
                return false;
            }
            auto e = r.Read();
            if (!e) {
                break;
            }
            if (e->is_(TokenKind::keyword)) {
                auto get_expr = [&](auto key, auto& c) -> std::shared_ptr<Expr> {
                    if (c) {
                        r.SetError(ErrorCode::double_decoration, key);
                        return nullptr;
                    }
                    auto e = r.ConsumeReadorEOF();
                    if (!e) {
                        return nullptr;
                    }
                    auto tree = get_tree();
                    auto res = binary(r, tree, mep);
                    if (!res) {
                        return nullptr;
                    }
                    return res;
                };
                auto get_cond = [&](auto e, auto key, auto& c) {
                    auto res = get_expr(key, c);
                    if (!res) {
                        return false;
                    }
                    c = std::make_shared<Condition>();
                    c->expr = res;
                    c->token = e;
                    return true;
                };
                if (e->has_("if")) {
                    if (!get_cond(e, "if", param->if_c)) {
                        return false;
                    }
                }
                else if (e->has_("bind")) {
                    if (!get_cond(e, "bind", param->bind_c)) {
                        return false;
                    }
                }
                else if (e->has_("default")) {
                    auto tmp = e;
                    auto res = get_expr("default", param->default_v);
                    if (!res) {
                        return false;
                    }
                    param->default_v = std::make_shared<Value>();
                    param->default_v->expr = res;
                    param->default_v->token = e;
                }
                else {
                    r.SetError(ErrorCode::expect_condition_keyword);
                    return false;
                }
                continue;
            }
            break;
        }
        return true;
    }

    bool parse_type(TokenReader& r, std::shared_ptr<Param>& param, Record& mep) {
        if (!mep.expand(r)) {
            return false;
        }
        auto e = r.ReadorEOF();
        if (!e) {
            return false;
        }
        if (e->is_(TokenKind::keyword)) {
            std::shared_ptr<Builtin> b;
            if (e->has_("int")) {
                b = std::make_shared<Int>();
            }
            else if (e->has_("byte")) {
                b = std::make_shared<Byte>();
            }
            else if (e->has_("bit")) {
                b = std::make_shared<Bit>();
            }
            else if (e->has_("uint")) {
                b = std::make_shared<UInt>();
            }
            else {
                r.SetError(ErrorCode::expect_type_keyword);
                return false;
            }
            b->token = e;
            r.Consume();
            if (!parse_length(r, b->length, mep)) {
                return false;
            }
            param = b;
        }
        else if (e->is_(TokenKind::identifiers)) {
            r.Consume();
            auto b = std::make_shared<Custom>();
            b->cargoname = e->to_string();
            param = b;
        }
        else {
            r.SetError(ErrorCode::expect_type);
            return false;
        }
        if (!parse_condition(r, param, mep)) {
            return false;
        }
        return true;
    }

    bool parse_baseinfo(TokenReader& r, BaseInfo& info, Record& mep) {
        if (!mep.expand(r)) {
            return false;
        }
        auto e = r.ReadorEOF();
        if (!e) {
            return false;
        }
        if (!e->is_(TokenKind::identifiers)) {
            r.SetError(ErrorCode::expect_id);
            return false;
        }
        info.selfname = e->to_string();
        if (!mep.expand(r)) {
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
        info.basename = e->to_string();
        r.Consume();
        return true;
    }

    bool parse_cargo(TokenReader& r, std::shared_ptr<Element>& elm, Record& mep) {
        auto e = r.ReadorEOF();
        if (!e) {
            return false;
        }
        if (!e->is_(TokenKind::keyword) && !e->has_("cargo")) {
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
        if (!mep.expand(r)) {
            return false;
        }
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
        if (!mep.expand(r)) {
            return false;
        }
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
            if (!mep.expand(r)) {
                return false;
            }
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