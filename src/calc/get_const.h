/*license*/
#pragma once
#include "../parse/parser/parse.h"
#include <extension_operator.h>
namespace binred {

    template <class Int>
    std::pair<Int, bool> get_const_int(std::shared_ptr<Expr>& expr) {
        if (!expr) {
            return {0, false};
        }
        if (expr->kind == ExprKind::number) {
            Int int_v;
            commonlib2::Reader(expr->v) >> int_v;
            return {int_v, true};
        }
        else if (expr->kind == ExprKind::op) {
            auto left = get_const_int<Int>(expr->left);
            if (!left.second) {
                return {0, false};
            }
            auto right = get_const_int<Int>(expr->right);
            if (!right.second) {
                return {0, false};
            }
            auto lv = left.first, rv = right.first;
            if (expr->v == "+") {
                return {lv + rv, true};
            }
            else if (expr->v == "-") {
                return {lv - rv, true};
            }
            else if (expr->v == "*") {
                return {lv * rv, true};
            }
            else if (expr->v == "/") {
                if (rv == 0) {
                    return {0, false};
                }
                return {lv / rv, true};
            }
            else if (expr->v == "%") {
                if (rv == 0) {
                    return {0, false};
                }
                return {lv % rv, true};
            }
            else if (expr->v == "&") {
                return {lv & rv, true};
            }
            else if (expr->v == "|") {
                return {lv | rv, true};
            }
            else if (expr->v == "^") {
                return {lv ^ rv, true};
            }
            else if (expr->v == ">") {
                return {lv > rv, true};
            }
            else if (expr->v == "<") {
                return {lv < rv, true};
            }
            else if (expr->v == "==") {
                return {lv == rv, true};
            }
            else {
                return {0, false};
            }
        }
        else {
            return {0, false};
        }
    }
}  // namespace binred
