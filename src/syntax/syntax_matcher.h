/*
    binred - binary I/O code generator
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once
#include "syntax_parser.h"
#include "callback.h"
namespace binred {
    namespace syntax {
        struct FloatReadPoint {
            std::string str;
            std::shared_ptr<token_t> beforedot;
            std::shared_ptr<token_t> dot;
            std::shared_ptr<token_t> afterdot;
            std::shared_ptr<token_t> sign;
            std::shared_ptr<token_t> aftersign;

            std::shared_ptr<token_t>& exists() {
                if (beforedot) {
                    return beforedot;
                }
                else if (dot) {
                    return dot;
                }
                else if (afterdot) {
                    return afterdot;
                }
                else if (sign) {
                    return sign;
                }
                else {
                    return aftersign;
                }
            }
        };

        struct MatchingContext {
            friend struct SyntaxMatching;

           private:
            std::vector<std::string> scope;
            std::string token;
            std::string elm;
            TokenReader* r;
            std::weak_ptr<token_t> node;

            bool match_(const std::string& n) {
                return false;
            }

            template <class Str, class... Args>
            bool match_(const std::string& n, const Str& str, const Args&... a) {
                if (n == str) {
                    return true;
                }
                return match_(n, a...);
            }

           public:
            const std::string& current() const {
                return scope[scope.size() - 1];
            }

            template <class String, class... Str>
            bool is_under_and_not(const String& n, const Str&... a) {
                for (auto i = scope.size() - 1; i < scope.size(); i--) {
                    if (scope[i] == n) {
                        return true;
                    }
                    if (match_(n, a...)) {
                        return false;
                    }
                }
                return false;
            }

            bool is_under(const std::string& n) const {
                for (auto i = scope.size() - 1; i < scope.size(); i--) {
                    if (scope[i] == n) {
                        return true;
                    }
                }
                return false;
            }

            bool is_type(const std::string& n) const {
                return n == elm;
            }

            bool is_token(const std::string& n) const {
                return n == token;
            }

            const TokenReader& get_reader() const {
                return *r;
            }

            const std::string& get_token() const {
                return token;
            }

            const std::string& get_type() const {
                return elm;
            }
        };

        struct SyntaxMatching {
            using holder_t = std::vector<std::shared_ptr<Syntax>>;
            SyntaxParser p;
            Callback<void, const MatchingContext&> cb;

           private:
            MatchingContext ctx;

           public:
            void callback(std::shared_ptr<token_t>& relnode, TokenReader& r, const std::string& token, const std::string& elm) {
                if (cb) {
                    ctx.token = token;
                    ctx.elm = elm;
                    ctx.r = &r;
                    ctx.node = relnode;
                    cb(ctx);
                }
            }

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
                callback(e, r, value, e->is_(TokenKind::symbols) ? "SYMBOL" : "KEYWORD");
                r.Consume();
                return 1;
            }

            int parse_or(TokenReader& r, std::shared_ptr<OrSyntax>& v, int& idx) {
                std::string errs;
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
                    errs += p.errmsg + "\n";
                }
                p.errmsg = errs;
                return 0;
            }

            int parse_ref(TokenReader& r, std::shared_ptr<Syntax>& v) {
                auto found = p.syntax.find(v->token->to_string());
                if (found == p.syntax.end()) {
                    p.errmsg = "syntax " + v->token->to_string() + " is not defined";
                    return -1;
                }
                ctx.scope.push_back(found->first);
                auto cr = r.FromCurrent();
                auto res = parse_on_vec(cr, found->second);
                if (res > 0) {
                    if (r.current == cr.current) {
                        p.errmsg = "infinity loop detected. please fix definitions especially around ? or *";
                        return -1;
                    }
                    r.current = cr.current;
                }
                ctx.scope.pop_back();
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

            void get_floatpoint(TokenReader& cr, FloatReadPoint& pt) {
                cr.Read();
                while (true) {
                    auto e = cr.Get();
                    if (!e) {
                        break;
                    }
                    if (!pt.dot && !pt.sign && e->has_(".")) {
                        pt.dot = e;
                        pt.str += e->to_string();
                        cr.Consume();
                        continue;
                    }
                    if (!pt.sign && (e->has_("+") || e->has_("-"))) {
                        pt.sign = e;
                        pt.str += e->to_string();
                        cr.Consume();
                        continue;
                    }
                    if (!e->is_(TokenKind::identifiers)) {
                        break;
                    }
                    if (!pt.dot && !pt.sign && !pt.beforedot) {
                        pt.beforedot = e;
                    }
                    else if (pt.dot && !pt.afterdot) {
                        pt.afterdot = e;
                    }
                    else if (pt.sign && !pt.aftersign) {
                        pt.aftersign = e;
                    }
                    else {
                        break;
                    }
                    pt.str += e->to_string();
                    cr.Consume();
                }
            }

            int parse_float(TokenReader& r) {
                auto cr = r.FromCurrent();
                FloatReadPoint pt;
                get_floatpoint(cr, pt);
                if (pt.str.size() == 0) {
                    p.errmsg = "expected number but not";
                    return 0;
                }
                int base = 10;
                int i = 0;
                if (pt.dot && !pt.beforedot && !pt.afterdot) {
                    p.errmsg = "invalid float number format";
                    return 0;
                }
                if (pt.str.starts_with("0x") || pt.str.starts_with("0X")) {
                    if (pt.str.size() >= 3 && pt.str[2] == '.' && !pt.afterdot) {
                        p.errmsg = "invalid hex float fromat. token is " + pt.str;
                        return 0;
                    }
                    base = 16;
                    i = 2;
                }
                int allowed = false;
                check_int_str(pt.str, i, base, allowed);
                if (pt.str.size() == allowed) {
                    if (!pt.beforedot || pt.dot || pt.sign) {
                        p.errmsg = "parser is broken";
                        return -1;
                    }
                    r.current = pt.beforedot->get_next();
                    callback(pt.exists(), r, pt.str, "INTEGER");
                    return 1;
                }
                if (pt.str[allowed] == '.') {
                    if (!pt.dot) {
                        p.errmsg = "parser is broken";
                        return -1;
                    }
                    i = allowed + 1;
                }
                check_int_str(pt.str, i, base, allowed);
                if (pt.str.size() == allowed) {
                    if (pt.sign) {
                        p.errmsg = "parser is broken";
                        return -1;
                    }
                    if (pt.afterdot) {
                        r.current = pt.afterdot->get_next();
                    }
                    else if (pt.dot) {
                        r.current = pt.dot->get_next();
                    }
                    else {
                        p.errmsg = "parser is broken";
                        return -1;
                    }
                    callback(pt.exists(), r, pt.str, "NUMBER");
                    return 1;
                }
                if (base == 16) {
                    if (pt.str[allowed] != 'p' && pt.str[allowed] != 'P') {
                        p.errmsg = "invalid hex float format. token is " + pt.str;
                        return 0;
                    }
                }
                else {
                    if (pt.str[allowed] != 'e' && pt.str[allowed] != 'E') {
                        p.errmsg = "invalid float format. token is " + pt.str;
                        return 0;
                    }
                }
                i = allowed + 1;
                if (pt.str.size() > i) {
                    if (pt.str[i] == '+' || pt.str[i] == '-') {
                        if (!pt.sign) {
                            p.errmsg = "parser is broken";
                            return -1;
                        }
                        i++;
                    }
                }
                if (pt.str.size() <= i) {
                    p.errmsg = "invalid float format. token is " + pt.str;
                    return 0;
                }
                check_int_str(pt.str, i, 10, allowed);
                if (pt.str.size() != allowed) {
                    p.errmsg = "invalid float format. token is " + pt.str;
                    return 0;
                }
                if (pt.aftersign) {
                    r.current = pt.aftersign->get_next();
                }
                else if (pt.afterdot) {
                    r.current = pt.afterdot->get_next();
                }
                else if (pt.beforedot) {
                    r.current = pt.beforedot->get_next();
                }
                else {
                    p.errmsg = "parser is broken";
                    return -1;
                }
                callback(pt.exists(), r, pt.str, "NUMBER");
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
                    callback(e, cr, e->to_string(), "ID");
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
                        return 0;
                    }
                    callback(e, cr, e->to_string(), "INTEGER");
                    cr.Consume();
                }
                else if (v->token->has_("NUMBER")) {
                    if (auto res = parse_float(cr); res <= 0) {
                        return res;
                    }
                }
                else if (v->token->has_("STRING")) {
                    auto e = cr.ReadorEOF();
                    if (!e) {
                        p.errmsg = "unexpected EOF. expect string";
                        return 0;
                    }
                    if (!e->has_("\"") && !e->has_("'") && !e->has_("`")) {
                        p.errmsg = "expected string but token is " + e->to_string();
                        return 0;
                    }
                    auto startvalue = e->to_string();
                    auto start = e;
                    e = cr.ConsumeGetorEOF();
                    if (!e) {
                        p.errmsg = "unexpected EOF. expect string";
                        return 0;
                    }
                    auto value = e->to_string();
                    e = cr.ConsumeGetorEOF();
                    if (!e) {
                        p.errmsg = "unexpected EOF. expect end of string " + startvalue;
                        return 0;
                    }
                    if (!e->has_(startvalue)) {
                        p.errmsg = "expect " + startvalue + " but token is " + e->to_string();
                    }
                    callback(start, cr, value, "STRING");
                    cr.Consume();
                }
                else {
                    p.errmsg = "unimplemented " + v->token->to_string();
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
                                return 0;
                            }
                            break;
                        }
                        else if (res < 0) {
                            return -1;
                        }
                        if (v->repeat) {
                            repeating = true;
                            continue;
                        }
                        break;
                    }
                    return 1;
                };
                for (auto& v : vec) {
                    switch (v->type) {
                        case SyntaxType::literal: {
                            if (auto e = call_v(
                                    [this](auto& r, auto& v) {
                                        return parse_literal(r, v);
                                    },
                                    v);
                                e <= 0) {
                                return e;
                            }
                            break;
                        }
                        case SyntaxType::ref: {
                            if (auto e = call_v(
                                    [this](auto& r, auto& v) {
                                        return parse_ref(r, v);
                                    },
                                    v);
                                e <= 0) {
                                return e;
                            }
                            break;
                        }
                        case SyntaxType::or_: {
                            auto ptr = std::static_pointer_cast<OrSyntax>(v);
                            if (!ptr->once_each) {
                                if (auto e = call_v(
                                        [&](auto& r, auto& v) {
                                            int index = 0;
                                            return parse_or(r, v, index);
                                        },
                                        ptr);
                                    e <= 0) {
                                    return e;
                                }
                            }
                            else {
                                std::set<int> already_set;
                                if (auto e = call_v(
                                        [&](auto& r, auto& v) {
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
                                        ptr);
                                    e <= 0) {
                                    return e;
                                }
                            }
                            break;
                        }
                        case SyntaxType::keyword: {
                            if (auto e = call_v(
                                    [this](auto& r, auto& v) {
                                        return parse_keyword(r, v);
                                    },
                                    v);
                                e <= 0) {
                                return e;
                            }
                            break;
                        }
                        default:
                            p.errmsg = "unimplemented";
                            return -1;
                    }
                }
                return 1;
            }

            int parse_follow_syntax() {
                auto found = p.syntax.find("ROOT");
                if (found == p.syntax.end()) {
                    p.errmsg = "need ROOT syntax element";
                    return false;
                }
                ctx.scope.clear();
                ctx.scope.push_back("ROOT");
                auto r = p.get_reader();
                auto res = parse_on_vec(r, found->second);
                if (res > 0) {
                    p.errmsg.clear();
                }
                return res;
            }
        };
    }  // namespace syntax
}  // namespace binred