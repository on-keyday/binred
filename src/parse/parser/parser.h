/*license*/
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
                    "{",
                    "}",
                    ":",
                    "-",
                    "+",
                    "&",
                    "==",
                    "|",
                    "$",
                    ">",
                    "<",
                    "/*",
                    "*/",
                    "//",
                    "/",
                    "*",
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
                    "endian",
                    "if",
                    "macro",
                    "bind",
                    "read",
                    "write",
                    "pop",
                    "push",
                    "transfer",
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
        unexpected_EOF,
        unexpected_line,
        expect_definition_keyword,
        expect_condition_keyword,
        undefined_macro,
        unexpected_symbol,
        unimplemented,
    };

    struct TokenReader {
        std::shared_ptr<token> root;
        std::shared_ptr<token> current;
        ErrorCode code = ErrorCode::none;
        const char* additional = nullptr;

        TokenReader() {}

        TokenReader(std::shared_ptr<token> tok)
            : root(tok), current(tok) {}

        bool is_EOF() {
            return current == nullptr;
        }

        void SeekRoot() {
            current = root;
        }

        void SetError(ErrorCode code, const char* addtional = nullptr) {
            this->code = code;
            this->additional = addtional;
        }

        std::shared_ptr<token> Read() {
            while (current &&
                   (current->is_nodisplay() ||
                    current->has_("/*") || current->has_("*/") || current->has_("//") ||
                    current->is_(TokenKind::comments))) {
                current = current->get_next();
            }
            return current;
        }

        std::shared_ptr<token> ReadorEOF() {
            auto ret = Read();
            if (!ret) {
                SetError(ErrorCode::unexpected_EOF);
            }
            return ret;
        }

        bool Consume() {
            if (current) {
                current = current->get_next();
                return true;
            }
            return false;
        }

        std::shared_ptr<token> ConsumeReadorEOF() {
            if (!Consume()) {
                SetError(ErrorCode::unexpected_EOF);
                return nullptr;
            }
            return ReadorEOF();
        }

        std::shared_ptr<token> GetorEOF() {
            if (!current) {
                SetError(ErrorCode::unexpected_EOF);
            }
            return current;
        }
        std::shared_ptr<token> Get() {
            return current;
        }
    };
}  // namespace binred
