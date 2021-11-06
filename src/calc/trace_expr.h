/*license*/
#pragma once
#include "../parse/parse.h"
#include <callback_invoker.h>
namespace binred {

    template <class Translate = bool (*)(std::string&, std::shared_ptr<Expr>)>
    std::string trace_expr(std::shared_ptr<Expr>& e, Translate&& translate = Translate()) {
        if (!e) {
            return std::string();
        }
        std::string ret;
        if (e->left) {
            ret += trace_expr(e->left);
            ret += " ";
        }
        if (!commonlib2::invoke_cb<Translate, bool>::invoke(std::move(translate), ret, e)) {
        }
        else {
            ret += e->v;
        }
        ret += " ";
        if (e->right) {
            ret += trace_expr(e->right);
            ret += " ";
        }
        return ret;
    }
}  // namespace binred