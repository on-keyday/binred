/*license*/
#pragma once
#include "parse_alias.h"
#include "parse_cargo.h"
#include "parse_macro.h"
#include "parse_read.h"

namespace binred {
    using ParseResult = std::vector<std::shared_ptr<Element>>;
    template <class Buf>
    bool parse_binred(commonlib2::Reader<Buf>& r, TokenReader& red, Record& mep, ParseResult& result) {
        TokenGetter gt;
        if (!gt.parse(r)) {
            red.SetError(ErrorCode::invalid_comment);
            return false;
        }
        red = TokenReader(gt.parser.GetParsed());
        while (!red.is_EOF()) {
            auto e = red.Read();
            if (!e) {
                break;
            }
            if (!e->is_(TokenKind::keyword)) {
                red.SetError(ErrorCode::expect_keyword);
                return false;
            }
            std::shared_ptr<Element> elm;
            if (e->has_("cargo")) {
                if (!parse_cargo(red, elm, mep)) {
                    return false;
                }
            }
            else if (e->has_("alias")) {
                if (!parse_alias(red, elm, mep)) {
                    return false;
                }
            }
            else if (e->has_("macro")) {
                if (!parse_macro(red, elm, mep)) {
                    return false;
                }
            }
            else if (e->has_("read")) {
                if (!parse_read(red, elm, mep)) {
                    return false;
                }
            }
            else {
                red.SetError(ErrorCode::expect_definition_keyword);
                return false;
            }
            result.push_back(elm);
        }
        return true;
    }
}  // namespace binred