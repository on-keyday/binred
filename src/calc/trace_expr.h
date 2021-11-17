/*
    binred - binary reader code generator
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once
#include "../parse/parser/parse.h"
#include <callback_invoker.h>
namespace binred {

    enum class TraceMode {
        normal,
        lisp,
    };

    template <class Translate = bool (*)(std::string&, std::shared_ptr<Expr>, TraceMode)>
    std::string trace_expr(std::shared_ptr<Expr>& e, Translate&& translate = Translate(), TraceMode mode = TraceMode::normal) {
        if (!e) {
            return std::string();
        }
        std::string ret;
        if (mode == TraceMode::normal) {
            if (e->left) {
                ret += " ";
                ret += trace_expr(e->left, std::forward<Translate>(translate), mode);
                ret += " ";
            }
            if (!commonlib2::Invoker<Translate, bool, false>::invoke(std::forward<Translate>(translate), ret, e, mode)) {
                ret += e->v;
            }
            if (e->right) {
                ret += " ";
                ret += trace_expr(e->right, std::forward<Translate>(translate), mode);
            }
        }
        else if (mode == TraceMode::lisp) {
            if (!commonlib2::Invoker<Translate, bool, false>::invoke(std::forward<Translate>(translate), ret, e, mode)) {
                ret += e->v;
            }
            if (e->left) {
                ret += " ";
                ret += trace_expr(e->left, std::forward<Translate>(translate), mode);
                ret += " ";
            }
            if (e->right) {
                ret += " ";
                ret += trace_expr(e->right, std::forward<Translate>(translate), mode);
            }
        }
        return ret;
    }
}  // namespace binred
