/*
    binred - binary I/O code generator
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once
#include <tokenparser/tokenparser.h>
#include <vector>
#include <string>
#include "../struct/element.h"

namespace binred {
    using namespace commonlib2::tokenparser;

    struct TokenGetter {
        using Parser = TokenParser<std::vector<std::string>, std::string>;
        Parser parser;

        TokenGetter() {
            parser.SetSymbolAndKeyWord(
                {
                    "==",
                    "<=",
                    ">=",
                    "/*",
                    "*/",
                    "//",
                    "{",
                    "}",
                    ":",
                    "-",
                    "+",
                    "&",
                    "*",
                    "/",
                    "|",
                    "$",
                    ">",
                    "<",
                    "\\",
                    ",",
                    "(",
                    ")",
                    "!=",
                    "!",
                    "=",
                    "[",
                    "]",
                },
                {
                    "cargo",
                    "alias",
                    "if",
                    "macro",
                    "bind",
                    "read",
                    "write",
                    "pop",
                    "push",
                    "transfer",
                    "switch",
                    "default",
                    "int",
                    "uint",
                    "bit",
                    "byte",
                    "string",
                    "call",
                    "if",
                    "elif",
                    "else",
                    "def",
                    "extern",
                    "complex",
                    "case",
                    "test",
                    "expand",
                    "nilable",
                    "nil",
                    "true",
                    "false",
                    "libname",
                });
        }

        template <class Reader>
        MergeErr parse(Reader& r) {
            MergeRule<std::string> rule;
            rule.begin_comment = "/*";
            rule.end_comment = "*/";
            rule.oneline_comment = "//";
            return parser.ReadAndMerge(r, rule);
        }
    };

    enum class ErrorCode {
        none,
        invalid_comment,
        expect_id,
        expect_keyword,
        expect_symbol,
        expect_type_keyword,
        expect_type,
        expect_number,
        expect_primary,
        double_decoration,
        multiple_variable,
        multiple_alias,
        multiple_macro,
        multiple_cargo,
        multiple_default,
        unexpected_EOF,
        unexpected_line,
        unexpected_keyword,
        unexpected_symbol,
        expect_definition_keyword,
        expect_condition_keyword,
        undefined_macro,
        unimplemented,
    };

    struct TokenReader : TokenReaderBase<std::string> {
        //std::shared_ptr<token_t> root;
        //std::shared_ptr<token_t> current;
        ErrorCode code = ErrorCode::none;
        const char* additional = nullptr;

        using TokenReaderBase<std::string>::TokenReaderBase;

        void SetError(ErrorCode code, const char* addtional = nullptr) {
            this->code = code;
            this->additional = addtional;
        }

        void SetEOF() override {
            SetError(ErrorCode::unexpected_EOF);
        }

        bool is_IgnoreSymbol() override {
            return current->has_("/*") || current->has_("*/") || current->has_("//");
        }
    };
}  // namespace binred
