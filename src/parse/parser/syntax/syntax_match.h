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
            SyntaxType type;
            std::shared_ptr<token_t> token;
            bool repeat = false;
            bool ifexists = false;
        };

        struct RefSyntax : Syntax {
            std::string other;
        };

        struct OrSyntax : Syntax {
            std::vector<std::shared_ptr<Syntax>> syntax;
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
            std::map<std::string, std::vector<std::shared_ptr<Syntax>>> syntax;
            std::string errmsg;
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
                e = r.ConsumeReadorEOF();
                if (!e) {
                    return false;
                }
                if (!e->has_(":=")) {
                    errmsg = "expect symbol := but " + e->to_string();
                    return false;
                }
                r.Consume();
                e = r.ReadorEOF();
            }
        };
    }  // namespace syntax
}  // namespace binred