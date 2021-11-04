/*license*/
#pragma once
#include "../parse/parse.h"
namespace binred {
    std::string trace_expr(std::shared_ptr<Expr>& e) {
        if (!e) {
            return std::string();
        }
        std::string ret;
        if (e->left) {
            ret += trace_expr(e->left);
            ret += " ";
        }
        ret += e->v;
        ret += " ";
        if (e->right) {
            ret += trace_expr(e->right);
            ret += " ";
        }
        return ret;
    }
}  // namespace binred