#pragma once
#include "parser.h"
#include "../struct/element.h"
#include <char_judge.h>
#include "../struct/record.h"
namespace binred {

    struct TreeDepth {
        std::vector<std::string> expected;
        TreeDepth(std::initializer_list<std::string> list)
            : expected(list.begin(), list.end()) {}
    };

    struct Tree {
        std::vector<TreeDepth> depth;
        size_t index = 0;

        Tree(std::initializer_list<TreeDepth> list)
            : depth(list.begin(), list.end()) {}
        bool expect(std::shared_ptr<token>& r, std::string& expected) {
            if (!r) return false;
            for (auto& s : depth[index % depth.size()].expected) {
                if (r->is_(TokenKind::symbols) && r->has_(s)) {
                    expected = s;
                    return true;
                }
            }
            return false;
        }
        void increment() {
            index++;
        }
        void decrement() {
            index--;
        }

        bool is_end() {
            return index != 0 && (index % depth.size() == 0);
        }
    };

    bool read_idlist(TokenReader& r, std::string& idname) {
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
        if (e->is_(TokenKind::symbols) && e->has_("$")) {
            r.Consume();
            ExprKind kind = ExprKind::ref;
            std::string idname;
            if (!read_idlist(r, idname)) {
                return nullptr;
            }
            ret->kind = kind;
            ret->v = idname;
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

    Tree get_tree() {
        Tree ret{
            {"==", ">", "<"},
            {"+", "-", "&", "|"},
            {"*", "/", "%"},
        };
        return ret;
    }
}  // namespace binred
