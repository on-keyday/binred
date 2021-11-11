#pragma once
#include "parser.h"
#include "../struct/element.h"
#include <char_judge.h>
#include "../struct/record.h"
namespace binred {

    std::shared_ptr<CallExpr> parse_callexpr(TokenReader& r, Record& rec);

    bool read_idname(TokenReader& r, std::string& idname) {
        auto e = r.ReadorEOF();
        if (!e) return false;
        if (!e->is_(TokenKind::identifiers)) {
            r.SetError(ErrorCode::expect_id);
            return false;
        }
        idname = e->to_string();
        r.Consume();
        while (auto next = e->get_next()) {
            if (next->is_(TokenKind::symbols) && next->has_(".")) {
                e = r.ConsumeReadorEOF();
                if (!e) {
                    return false;
                }
                if (!e->is_(TokenKind::identifiers)) {
                    r.SetError(ErrorCode::expect_id);
                    return false;
                }
                idname += ".";
                idname += e->to_string();
                r.Consume();
                continue;
            }
            break;
        }
        return true;
    }

    std::shared_ptr<Expr> binary(TokenReader& r, Tree& t, Record& mep);

    std::shared_ptr<Expr> primary(TokenReader& r, Tree& t, Record& mep) {
        if (!mep.expand(r)) {
            return nullptr;
        }
        auto e = r.ReadorEOF();
        if (!e) {
            return nullptr;
        }
        auto ret = std::make_shared<Expr>();
        ret->token = e;
        if (e->is_(TokenKind::symbols)) {
            if (e->has_("$")) {
                r.Consume();
                ExprKind kind = ExprKind::ref;
                std::string idname;
                if (!read_idname(r, idname)) {
                    return nullptr;
                }
                ret->kind = kind;
                ret->v = idname;
            }
            else if (e->has_("(")) {
                r.Consume();
                auto tmp = t.get_index();
                t.set_index(0);
                ret = binary(r, t, mep);
                if (!ret) {
                    return nullptr;
                }
                e = r.ReadorEOF();
                if (!e) {
                    return nullptr;
                }
                if (!e->has_(")")) {
                    r.SetError(ErrorCode::expect_symbol, ")");
                }
                t.set_index(tmp);
                r.Consume();
            }
            else {
                r.SetError(ErrorCode::unexpected_symbol);
                return nullptr;
            }
        }
        else if (e->is_(TokenKind::identifiers)) {
            r.Consume();
            auto id = e->identifier();
            auto& idv = id->get_identifier();
            int i = 0;
            int base = 10;
            if (idv.size() >= 3 && idv.starts_with("0x") || idv.starts_with("0X")) {
                i = 2;
                base = 16;
            }
            for (; i < idv.size(); i++) {
                if (base == 16) {
                    if (!commonlib2::is_hex(idv[i])) {
                        r.SetError(ErrorCode::expect_number);
                        return nullptr;
                    }
                }
                else {
                    if (!commonlib2::is_digit(idv[i])) {
                        r.SetError(ErrorCode::expect_number);
                        return nullptr;
                    }
                }
            }
            ret->v = idv;
            ret->kind = ExprKind::number;
        }
        else if (e->is_(TokenKind::keyword)) {
            if (e->has_("call")) {
                auto tmp = t.get_index();
                t.set_index(0);
                ret = parse_callexpr(r, mep);
                if (!ret) {
                    return nullptr;
                }
                t.set_index(tmp);
            }
            else {
                r.SetError(ErrorCode::unexpected_keyword);
                return nullptr;
            }
        }
        else {
            r.SetError(ErrorCode::expect_primary);
            return nullptr;
        }
        return ret;
    }

    std::shared_ptr<Expr> binary(TokenReader& r, Tree& t, Record& mep) {
        if (t.is_end()) {
            return primary(r, t, mep);
        }
        std::shared_ptr<Expr> ret;
        auto call = [&](auto& res) {
            t.increment();
            res = binary(r, t, mep);
            t.decrement();
        };
        call(ret);
        if (!ret) {
            return nullptr;
        }
        while (true) {
            auto tok = r.Read();
            if (!tok) {
                break;
            }
            std::string expected;
            if (t.expect(tok, expected)) {
                r.Consume();
                auto tmp = std::make_shared<Expr>();
                tmp->token = tok;
                tmp->kind = ExprKind::op;
                tmp->v = expected;
                tmp->left = ret;
                ret = tmp;
                call(ret->right);
                if (!ret->right) {
                    return nullptr;
                }
                continue;
            }
            break;
        }
        return ret;
    }

}  // namespace binred
