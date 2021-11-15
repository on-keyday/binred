/*license*/
#pragma once
#include "../struct/record.h"
#include "parse_tree.h"

namespace binred {
    bool parse_length(TokenReader& r, std::shared_ptr<Length>& length, Record& mep) {
        EXPAND_MACRO(mep)
        //auto tree = get_tree();
        auto res = binary(r, mep.get_tree(), mep);
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
            EXPAND_MACRO(mep)
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
                    //auto tree = get_tree();
                    auto res = binary(r, mep.get_tree(), mep);
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
                else if (e->has_("expand")) {
                    if (param->expand) {
                        r.SetError(ErrorCode::double_decoration, "expand");
                        return false;
                    }
                    r.Consume();
                    param->expand = true;
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
        EXPAND_MACRO(mep)
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
}  // namespace binred
