/*
    binred - binary I/O code generator
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once

#include "../parser.h"
#include "../parse_tree.h"

#include <vector>
#include <set>

namespace binred {
    namespace syntax {
        using Parser = TokenParser<std::vector<std::string>, std::string>;
        enum class SyntaxType {
            literal,
            ref,
            keyword,
            or_,
        };

        struct Syntax {
            Syntax(SyntaxType t)
                : type(t) {}
            SyntaxType type;
            std::shared_ptr<token_t> token;
            bool repeat = false;
            bool ifexists = false;
        };

        struct OrSyntax : Syntax {
            using Syntax::Syntax;
            std::vector<std::vector<std::shared_ptr<Syntax>>> syntax;
            bool once_each = false;
        };

        struct TokenReader : TokenReaderBase<std::string> {
            using TokenReaderBase<std::string>::TokenReaderBase;
            bool igline = true;
            void SetIgnoreLine(bool t) {
                igline = t;
            }
            bool is_IgnoreToken() override {
                if (!igline && current->is_(TokenKind::line)) {
                    return false;
                }
                return is_DefaultIgnore() || current->has_("#");
            }

            TokenReader FromCurrent() {
                TokenReader ret(current);
                ret.igline = igline;
                return ret;
            }
        };

        struct SyntaxParser {
            TokenReader r;
            using holder_t = std::vector<std::shared_ptr<Syntax>>;
            std::map<std::string, holder_t> syntax;
            std::string errmsg;
            Parser parser;
            std::set<std::string> already_found;

            bool is_symbol(const std::string& str) {
                for (auto& c : str) {
                    auto s = (std::uint8_t)c;
                    if (std::isgraph(s) && !std::isalnum(s)) {
                        continue;
                    }
                    return false;
                }
                return true;
            }

            bool read_syntaxline(holder_t& stx, bool bracket = false) {
                while (true) {
                    auto e = r.ReadorEOF();
                    r.SetIgnoreLine(false);
                    std::shared_ptr<Syntax> ptr;
                    if (!e || e->is_(TokenKind::line)) {
                        r.SetIgnoreLine(true);
                        break;
                    }
                    if (bracket && (e->has_("]") || e->has_("|"))) {
                        break;
                    }
                    if (e->has_("\"")) {
                        e = r.ConsumeGetorEOF();
                        if (!e) {
                            errmsg = "unexpected EOF, expect string";
                            return false;
                        }
                        if (!e->is_(TokenKind::comments)) {
                            errmsg = "parser is broken";
                            return false;
                        }
                        ptr = std::make_shared<Syntax>(SyntaxType::literal);
                        ptr->token = e;
                        e = r.ConsumeGetorEOF();
                        if (!e) {
                            errmsg = "unexpected EOF,expect \"";
                            return false;
                        }
                        if (!e->has_("\"")) {
                            errmsg = "expect \" but " + e->to_string();
                            return false;
                        }
                        r.Consume();
                        if (auto found = already_found.insert(ptr->token->to_string()); found.second) {
                            if (is_symbol(*found.first)) {
                                parser.GetSymbols().Register(*found.first);
                            }
                            else {
                                parser.GetKeyWords().Register(*found.first);
                            }
                        }
                    }
                    else if (e->is_(TokenKind::keyword)) {
                        ptr = std::make_shared<Syntax>(SyntaxType::keyword);
                        ptr->token = e;
                        r.Consume();
                    }
                    else if (e->is_(TokenKind::identifiers)) {
                        ptr = std::make_shared<Syntax>(SyntaxType::ref);
                        ptr->token = e;
                        r.Consume();
                    }
                    else if (e->has_("[")) {
                        r.Consume();
                        auto tmp = std::make_shared<OrSyntax>(SyntaxType::or_);
                        tmp->token = e;
                        while (true) {
                            holder_t holder;
                            if (!read_syntaxline(holder, true)) {
                                return false;
                            }
                            if (holder.size() == 0) {
                                errmsg = "need more than one syntax element";
                                return false;
                            }
                            tmp->syntax.push_back(std::move(holder));
                            e = r.ReadorEOF();
                            if (!e) {
                                errmsg = "unexpected EOF. expect | or ]";
                                return false;
                            }
                            if (e->is_(TokenKind::line)) {
                                errmsg = "unexpected EOL, expect | or ]";
                                return false;
                            }
                            else if (e->has_("]")) {
                                r.Consume();
                                break;
                            }
                            else if (e->has_("|")) {
                                r.Consume();
                            }
                            else {
                                errmsg = "unexpected token " + e->to_string() + ". expect ] or |";
                                return false;
                            }
                        }
                        e = r.Read();
                        if (e && e->has_("$")) {
                            tmp->once_each = true;
                            r.Consume();
                        }
                        ptr = tmp;
                    }
                    else {
                        errmsg = "unexpected token " + e->to_string() + ". expect literal or identifier";
                        return false;
                    }
                    while (true) {
                        e = r.Read();
                        if (!ptr->ifexists && e->has_("?")) {
                            ptr->ifexists = true;
                            r.Consume();
                            continue;
                        }
                        else if (!ptr->repeat && e->has_("*")) {
                            ptr->repeat = true;
                            r.Consume();
                            continue;
                        }
                        break;
                    }
                    stx.push_back(std::move(ptr));
                }
                return true;
            }

            bool parse_a_syntax() {
                auto e = r.ReadorEOF();
                if (!e) {
                    return false;
                }
                if (!e->is_(TokenKind::identifiers)) {
                    errmsg = "expect identifier for syntax name but " + e->to_string();
                    return false;
                }
                std::string name = e->to_string();
                e = r.ConsumeReadorEOF();
                if (!e) {
                    errmsg = "unexpected EOF. expect :=";
                    return false;
                }
                if (!e->has_(":=")) {
                    errmsg = "expect symbol := but " + e->to_string();
                    return false;
                }
                r.Consume();
                auto result = syntax.insert({name, holder_t()});
                if (!result.second) {
                    errmsg = "syntax " + name + " is already defined";
                    return false;
                }
                auto& stx = result.first->second;
                if (!read_syntaxline(stx)) {
                    return false;
                }
                return true;
            }

            bool operator()() {
                already_found.clear();
                errmsg.clear();
                parser = Parser();
                already_found.emplace(".");
                already_found.emplace("+");
                already_found.emplace("-");
                parser.GetSymbols() = {".", "+", "-"};
                while (true) {
                    if (!r.Read()) {
                        break;
                    }
                    if (!parse_a_syntax()) {
                        return false;
                    }
                }
                auto& keywords = parser.GetKeyWords().reg;
                auto& symbols = parser.GetSymbols().reg;
                auto sorter = [](const std::string& a, const std::string& b) {
                    if (a > b) {
                        if (a.size() > b.size()) {
                            return true;
                        }
                        return false;
                    }
                    return false;
                };
                std::sort(keywords.begin(), keywords.end(), sorter);
                std::sort(symbols.begin(), symbols.end(), sorter);
                return true;
            }

            template <class Reader>
            MergeErr parse(Reader& r) {
                MergeRule<std::string> rule;
                rule.oneline_comment = "#";
                rule.string_symbol[0].symbol = '"';
                return parser.ReadAndMerge(r, rule);
            }

            auto get_reader() {
                return TokenReader(parser.GetParsed());
            }
        };

        struct SyntaxC {
            Parser parser;
            SyntaxC() {
                parser.SetSymbolAndKeyWord(
                    {
                        "\"",
                        "'",
                        "`",
                        ":=",
                        "?",
                        "[",
                        "]",
                        "*",
                        "|",
                        "#",
                        "$",
                    },
                    {
                        "ID",
                        "INTEGER",
                        "NUMBER",
                        "STRING",
                    });
            }

            template <class Reader>
            MergeErr parse(Reader& r) {
                MergeRule<std::string> rule;
                rule.oneline_comment = "#";
                rule.string_symbol[0].symbol = '"';
                return parser.ReadAndMerge(r, rule);
            }

            auto get_reader() {
                return TokenReader(parser.GetParsed());
            }

            auto get_compiler() {
                return SyntaxParser{get_reader()};
            }
        };
    }  // namespace syntax
}  // namespace binred