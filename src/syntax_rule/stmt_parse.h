/*
    binred - binary I/O code generator
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once

#include "stmt.h"
#include <callback.h>

namespace binred {
    namespace stmt {
        using SyntaxCb = commonlib2::Callback<cl2s::MatchingErr, const cl2s::MatchingContext&, bool&>;
        struct ParserCallback {
            bool is_set = false;
            bool result = false;
            bool change_cb = false;
            cl2s::MatchingStackInfo stack;
            const cl2s::MatchingContext* ctx = nullptr;
            SyntaxCb cb;

            bool set_ctx(const cl2s::MatchingContext& ctx) {
                this->ctx = &ctx;
                result = false;
                change_cb = false;
                if (!is_set) {
                    stack = ctx.get_stack();
                    is_set = true;
                    return true;
                }
                return false;
            }

            bool is_roolbacked() const {
                return ctx->is_rollbacked(stack);
            }

            bool is_current() const {
                return ctx->is_current(stack);
            }

            bool callcb() {
                if (cb) {
                    result = (bool)cb(*ctx, change_cb);
                    return true;
                }
            }

            bool is_cbsucceed() const {
                return result;
            }

            bool is_cbchange() const {
                return result && change_cb;
            }

            bool is_cbrollback() const {
                return !result && change_cb;
            }

            bool is_cbfailed() const {
                return !result && !change_cb;
            }

            bool broken() {
                ctx->set_errmsg("syntax parser is broken");
                return false;
            }

            bool rollback(bool& chcb) {
                chcb = true;
                return false;
            }

            bool finish(bool& chcb) {
                chcb = true;
                return true;
            }
        };

        template <class ExprP>
        struct FuncCall : ParserCallback {
            std::shared_ptr<CallExpr> call;
            bool operator()(const cl2s::MatchingContext& ctx, bool& chcb) {
                if (set_ctx(&ctx)) {
                    call = std::make_shared<CallExpr>();
                }
                if (is_roolbacked()) {
                    return broken();
                }
                if (is_current()) {
                    if (ctx.is_token("(")) {
                        return true;
                    }
                    else if (ctx.is_token(",") || ctx.is_token(")")) {
                        auto p = cb.template get_rawfunc<ExprP>();
                        if (!p) {
                            return broken();
                        }
                        call->args.push_back(std::move(p->expr));
                        if (ctx.is_token(")")) {
                            return finish(chcb);
                        }
                        return true;
                    }
                    else {
                        return broken();
                    }
                }
                if (!cb) {
                    cb = ExprP();
                }
                callcb();
                return result;
            }
        };

        struct ExprParser : ParserCallback {
            std::shared_ptr<Expr> expr;
            bool operator()(const cl2s::MatchingContext& ctx, bool& chcb) {
                set_ctx(ctx);
                ExprKind kind = ExprKind::id;
                auto set_to = [&](auto& ptr) {
                    ptr = std::make_shared<Expr>();
                    ptr->v = ctx.get_token();
                    ptr->kind = kind;
                };
                auto set_prim = [&] {
                    if (!expr) {
                        set_to(expr);
                    }
                    else if (!expr->right) {
                        set_to(expr->right);
                    }
                    else {
                        return broken();
                    }
                    return true;
                };
                if (callcb()) {
                    if (is_cbchange()) {
                        auto p = cb.get_rawfunc<FuncCall<ExprParser>>();
                        if (!p) {
                            return broken();
                        }
                        p->call->left = expr;
                        expr = std::move(p->call);
                        return true;
                    }
                    else if (!is_cbsucceed()) {
                        return false;
                    }
                    return true;
                }
                if (ctx.is_current("FUNCCALL")) {
                    cb = FuncCall<ExprParser>();
                    callcb();
                    return result;
                }
            }
        };

    }  // namespace stmt
}  // namespace binred
