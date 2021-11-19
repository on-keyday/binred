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

namespace binred {
    namespace syntax {
        enum class SyntaxType {
            literal,
            ref,
            identifier,
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
            std::vector<std::vector<std::shared_ptr<Syntax>>> syntax;
        };

        struct SyntaxC {
            using Parser = TokenParser<std::vector<std::string>, std::string>;
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
                    },
                    {"ID"});
            }

            template <class Reader>
            MergeErr parse(Reader& r) {
                MergeRule<std::string> rule;
                rule.oneline_comment = "#";
                rule.string_symbol[0].symbol = '"';
                rule.string_symbol[0];
                return parser.ReadAndMerge(r, rule);
            }

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
            };

            auto get_reader() {
                return TokenReader(parser.GetParsed());
            }
        };

        struct ReadSyntax {
            SyntaxC::TokenReader r;
            using holder_t = std::vector<std::shared_ptr<Syntax>>;
            std::map<std::string, holder_t> syntax;
            std::string errmsg;

            bool read_syntaxline(holder_t& stx, bool bracket = false) {
                while (true) {
                    auto e = r.ReadorEOF();
                    std::shared_ptr<Syntax> ptr;
                    if (!e || e->is_(TokenKind::line)) {
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
                    }
                    else if (e->is_(TokenKind::keyword)) {
                        ptr = std::make_shared<Syntax>(SyntaxType::identifier);
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
                    }
                }
            }

            bool operator()() {
                auto e = r.ReadorEOF();
                if (!e) {
                    return false;
                }
                r.SetIgnoreLine(false);
                if (!e->is_(TokenKind::identifiers)) {
                    errmsg = "expect identifier for syntax name but " + e->to_string();
                    return false;
                }
                std::string name = e->to_string();
                e = r.ConsumeReadorEOF();
                if (!e) {
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
            }
        };
    }  // namespace syntax
}  // namespace binred