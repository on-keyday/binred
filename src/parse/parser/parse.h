/*
    binred - binary reader code generator
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once
#include "parse_alias.h"
#include "parse_cargo.h"
#include "parse_macro.h"
#include "parse_io.h"
#include "parse_tree.h"

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
        auto e = red.Read();
        if (e && e->is_(TokenKind::keyword) && e->has_("libname")) {
            red.Consume();
            if (!read_idname(red, mep.libname, true)) {
                return false;
            }
        }
        while (!red.is_EOF()) {
            e = red.Read();
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
                if (!parse_io<Read, true>(red, elm, mep)) {
                    return false;
                }
            }
            else if (e->has_("write")) {
                if (!parse_io<Write, false>(red, elm, mep)) {
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
