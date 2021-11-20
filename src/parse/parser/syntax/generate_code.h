/*
    binred - binary I/O code generator
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once
#include "syntax_match.h"
namespace binred {
    namespace syntax {
        struct Matching {
            using holder_t = std::vector<std::shared_ptr<Syntax>>;
            SyntaxParser p;

            int parse_literal(TokenReader& r, std::shared_ptr<Syntax>& v) {
                auto e = r.ReadorEOF();
                if (!e) {
                    p.errmsg = "unexpected EOF. expect " + v->token->to_string();
                    return 0;
                }
                auto value = v->token->to_string();
                if (!e->has_(value)) {
                    p.errmsg = "expected " + value + " but " + e->to_string();
                    return 0;
                }
                r.Consume();
                return 1;
            }

            int parse_or(TokenReader& r, std::shared_ptr<OrSyntax>& v, int& idx) {
                for (auto i = 0; i < v->syntax.size(); i++) {
                    auto cr = r.FromCurrent();
                    auto res = parse_on_vec(cr, v->syntax[i]);
                    if (res > 0) {
                        idx = i;
                        r.current = cr.current;
                        return 1;
                    }
                    else if (res < 0) {
                        return res;
                    }
                }
                return 0;
            }

            int parse_ref(TokenReader& r, std::shared_ptr<Syntax>& v) {
                auto found = p.syntax.find(v->token->to_string());
                if (found == p.syntax.end()) {
                    p.errmsg = "syntax " + v->token->to_string() + " is not defined";
                    return -1;
                }
                auto cr = r.FromCurrent();
                auto res = parse_on_vec(r, found->second);
                if (res > 0) {
                    r.current = cr.current;
                }
                return res;
            }

            bool check_int_str(auto& idv, int i, int base, int& allowed) {
                for (; i < idv.size(); i++) {
                    if (base == 16) {
                        if (!commonlib2::is_hex(idv[i])) {
                            allowed = i;
                            return true;
                        }
                    }
                    else {
                        if (!commonlib2::is_digit(idv[i])) {
                            allowed = i;
                            return true;
                        }
                    }
                }
                allowed = idv.size();
                return true;
            }

            bool parse_float(TokenReader& r, std::shared_ptr<Syntax>& v) {
                std::string str;
                auto cr = r.FromCurrent();
                std::shared_ptr<token_t> beforedot, dot, afterdot, sign, aftersign;
                cr.Read();
                while (true) {
                    auto e = cr.Get();
                    if (!e) {
                        break;
                    }
                    if (!dot && !sign && e->has_(".")) {
                        dot = e;
                        str += e->to_string();
                        cr.Consume();
                        continue;
                    }
                    if (!sign && (e->has_("+") || e->has_("-"))) {
                        sign = e;
                        str += e->to_string();
                        cr.Consume();
                        continue;
                    }
                    if (!e->is_(TokenKind::identifiers)) {
                        break;
                    }
                    if (!dot && !sign && !beforedot) {
                        beforedot = e;
                    }
                    else if (dot && !afterdot) {
                        afterdot = e;
                    }
                    else if (sign && !aftersign) {
                        aftersign = e;
                    }
                    else {
                        break;
                    }
                    str += e->to_string();
                    cr.Consume();
                }
                if (str.size() == 0) {
                    p.errmsg = "expected number but not";
                    return false;
                }
                int base = 10;
                int i = 0;
                if (dot && !beforedot && !afterdot) {
                    p.errmsg = "invalid float number format";
                    return false;
                }
                if (str.starts_with("0x") || str.starts_with("0X")) {
                    if (str.size() >= 3 && str[2] == '.' && !afterdot) {
                        p.errmsg = "invalid hex float fromat. token is " + str;
                        return false;
                    }
                    base = 16;
                    i = 2;
                }
                int allowed = false;
                check_int_str(str, i, base, allowed);
                if (str.size() == allowed) {
                    if (!beforedot || dot || sign) {
                        p.errmsg = "parser is broken";
                        return false;
                    }
                    r.current = beforedot->get_next();
                    return true;
                }
                if (str[allowed] == '.') {
                    if (!dot) {
                        p.errmsg = "parser is broken";
                        return false;
                    }
                    i = allowed + 1;
                }
                check_int_str(str, i, base, allowed);
                if (str.size() == allowed) {
                    if (sign) {
                        p.errmsg = "parser is broken";
                        return false;
                    }
                    if (afterdot) {
                        r.current = afterdot->get_next();
                    }
                    else if (dot) {
                        r.current = dot->get_next();
                    }
                    else {
                        p.errmsg = "parser is broken";
                        return false;
                    }
                    return true;
                }
                if (base == 16) {
                    if (str[allowed] != 'p' && str[allowed] != 'P') {
                        p.errmsg = "invalid hex float format. token is " + str;
                        return false;
                    }
                }
                else {
                    if (str[allowed] == 'e' || str[allowed] == 'E') {
                        p.errmsg = "invalid float format. token is " + str;
                        return false;
                    }
                }
                i = allowed + 1;
                if (str.size() > i) {
                    if (str[i] == '+' || str[i] == '-') {
                        if (!sign) {
                            p.errmsg = "parser is broken";
                            return false;
                        }
                        i++;
                    }
                }
                if (str.size() <= i) {
                    p.errmsg = "invalid float format. token is " + str;
                    return false;
                }
                check_int_str(str, i, 10, allowed);
                if (str.size() != allowed) {
                    p.errmsg = "invalid float format. token is " + str;
                    return false;
                }
                if (aftersign) {
                    r.current = aftersign->get_next();
                }
                else if (afterdot) {
                    r.current = afterdot->get_next();
                }
                else if (beforedot) {
                    r.current = beforedot->get_next();
                }
                else {
                    p.errmsg = "parser is broken";
                }
                return true;
            }

            int parse_keyword(TokenReader& r, std::shared_ptr<Syntax>& v) {
                auto cr = r.FromCurrent();
                auto check_integer = [&](auto& e) {
                    auto id = e->identifier();
                    auto& idv = id->get_identifier();
                    int i = 0;
                    int base = 10;
                    if (idv.size() >= 3 && idv.starts_with("0x") || idv.starts_with("0X")) {
                        i = 2;
                        base = 16;
                    }
                    int allowed = 0;
                    check_int_str(idv, i, base, allowed);
                    if (idv.size() != allowed) {
                        p.errmsg = "invalid integer." + idv;
                        return false;
                    }
                    return true;
                };
                if (v->token->has_("ID")) {
                    auto e = cr.ReadorEOF();
                    if (!e) {
                        p.errmsg = "unexpected EOF. expect identifier";
                    }
                    if (!e->is_(TokenKind::identifiers)) {
                        p.errmsg = "expect identifier but token is " + e->to_string();
                        return 0;
                    }
                    cr.Consume();
                }
                else if (v->token->has_("INTEGER")) {
                    auto e = cr.ReadorEOF();
                    if (!e) {
                        p.errmsg = "unexpected EOF. expect integer";
                        return 0;
                    }
                    if (!e->is_(TokenKind::identifiers)) {
                        p.errmsg = "expect integer but token is " + e->to_string();
                        return 0;
                    }
                    if (!check_integer(e)) {
                        return -1;
                    }
                    cr.Consume();
                }
                else if (v->token->has_("NUMBER")) {
                    parse_float(r, v);
                }
                else {
                    return -1;
                }
                r.current = cr.current;
                return 1;
            }

            int parse_on_vec(TokenReader& r, holder_t& vec) {
                auto call_v = [&](auto f, auto& v) {
                    bool repeating = false;
                    while (true) {
                        if (auto res = f(r, v); res == 0) {
                            if (!repeating && !v->ifexists) {
                                return false;
                            }
                            break;
                        }
                        else if (res < 0) {
                            return false;
                        }
                        if (v->repeat) {
                            repeating = true;
                            continue;
                        }
                        break;
                    }
                    return true;
                };
                for (auto& v : vec) {
                    switch (v->type) {
                        case SyntaxType::literal: {
                            if (!call_v([this](auto& r, auto& v) {
                                    return parse_literal(r, v);
                                },
                                        v)) {
                                return -1;
                            }
                        }
                        case SyntaxType::ref: {
                            if (!call_v([this](auto& r, auto& v) {
                                    return parse_ref(r, v);
                                },
                                        v)) {
                                return -1;
                            }
                            break;
                        }
                        case SyntaxType::or_: {
                            auto ptr = std::static_pointer_cast<OrSyntax>(v);
                            if (!ptr->once_each) {
                                if (!call_v([&](auto& r, auto& v) {
                                        int index = 0;
                                        return parse_or(r, v, index);
                                    },
                                            ptr)) {
                                    return -1;
                                }
                            }
                            else {
                                std::set<int> already_set;
                                if (!call_v([&](auto& r, auto& v) {
                                        int index = 0;
                                        auto res = parse_or(r, v, index);
                                        if (res > 0) {
                                            if (!already_set.insert(index).second) {
                                                p.errmsg = "index " + std::to_string(index) + " is already set";
                                                return -1;
                                            }
                                        }
                                        return res;
                                    },
                                            ptr)) {
                                    return -1;
                                }
                                return true;
                            }
                            break;
                        }
                        case SyntaxType::keyword: {
                            if (!call_v([this](auto& r, auto& v) {
                                    return parse_keyword(r, v);
                                },
                                        v)) {
                                return -1;
                            }
                            break;
                        }
                        default:
                            break;
                    }
                }
            }

            bool parse_follow_syntax() {
                auto found = p.syntax.find("ROOT");
                if (found == p.syntax.end()) {
                    p.errmsg = "need ROOT syntax element";
                    return false;
                }
                auto r = p.get_reader();
                return parse_on_vec(r, found->second);
            }
        };
    }  // namespace syntax
}  // namespace binred