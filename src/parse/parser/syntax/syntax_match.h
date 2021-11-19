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
        struct Syntax {
            std::shared_ptr<token_t> token;
            std::vector<std::shared_ptr<Syntax>> next;
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
        };

        struct TokenReader : TokenReaderBase<std::string> {
            using TokenReaderBase<std::string>::TokenReaderBase;

            bool is_IgnoreSymbol() override {
            }
        };
    }  // namespace syntax
}  // namespace binred