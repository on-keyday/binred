/*
    binred - binary I/O code generator
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once

#include "../struct/io.h"
#include "../struct/record.h"
#include "parse_tree.h"
#include "parser.h"
namespace binred {

    template <class CmdType>
    bool parse_push_or_pop(TokenReader& r, std::shared_ptr<Command>& cmd, Record& rec, const char* poppush) {
        auto e = r.ReadorEOF();
        if (!e) {
            return false;
        }
        if (!e->is_(TokenKind::keyword) || !e->has_(poppush)) {
            r.SetError(ErrorCode::expect_keyword, poppush);
            return false;
        }
        r.Consume();
        auto tmp = std::make_shared<CmdType>(std::move(e));
        //auto tree = get_tree();
        tmp->numpop = binary(r, rec.get_tree(), rec);
        if (!tmp->numpop) {
            return false;
        }
        e = r.Read();
        if (e && e->has_("$")) {
            r.Consume();
            if (!read_idname(r, tmp->refid)) {
                return false;
            }
        }
        cmd = tmp;
        return true;
    }

    std::shared_ptr<CallExpr> parse_callexpr(TokenReader& r, Record& rec) {
        auto e = r.ReadorEOF();
        if (!e) {
            return nullptr;
        }
        if (!e->is_(TokenKind::keyword) || !e->has_("call")) {
            r.SetError(ErrorCode::expect_keyword, "call");
            return nullptr;
        }
        r.Consume();
        auto expr = std::make_shared<CallExpr>();
        expr->token = e;
        std::string name;
        if (!read_idname(r, name)) {
            return nullptr;
        }
        expr->v = name;
        e = r.ReadorEOF();
        if (!e) {
            return nullptr;
        }
        if (!e->has_("(")) {
            r.SetError(ErrorCode::expect_symbol, "(");
            return nullptr;
        }
        r.Consume();
        while (true) {
            e = r.ReadorEOF();
            if (e->has_(")")) {
                r.Consume();
                break;
            }
            auto tmp = binary(r, rec.get_tree(), rec);
            if (!tmp) {
                return nullptr;
            }
            expr->args.push_back(tmp);
            e = r.ReadorEOF();
            if (!e) {
                return nullptr;
            }
            if (e->has_(",")) {
                r.Consume();
            }
        }
        return expr;
    }

    bool parse_callcommand(TokenReader& r, std::shared_ptr<Command>& cmd, Record& rec) {
        auto e = r.ReadorEOF();
        if (!e) {
            return false;
        }
        auto tmp = std::make_shared<CallCommand>(std::move(e));
        if (tmp->call = parse_callexpr(r, rec); !tmp->call) {
            return false;
        }
        return true;
    }

    bool parse_switch(std::shared_ptr<token_t>&& start, TokenReader& r, std::shared_ptr<Command>& cmd, Record& rec) {
        auto e = r.ReadorEOF();
        if (!e) {
            return false;
        }
        if (!e->is_(TokenKind::keyword) || !e->has_("switch")) {
            r.SetError(ErrorCode::expect_keyword, "switch");
            return false;
        }
        r.Consume();
        auto tmp = std::make_shared<TransferSwitch>(std::move(start));
        tmp->cond = binary(r, rec.get_tree(), rec);
        if (!tmp->cond) {
            return false;
        }
        e = r.ReadorEOF();
        if (!e) {
            return false;
        }
        if (!e->has_("{")) {
            return false;
        }
        r.Consume();
        while (true) {
            e = r.ReadorEOF();
            if (!e) {
                return false;
            }
            if (e->has_("}")) {
                r.Consume();
                break;
            }
            if (!e->is_(TokenKind::keyword)) {
                r.SetError(ErrorCode::expect_keyword, "case or default");
                return false;
            }
            if (e->has_("case")) {
                r.Consume();
                auto expr = binary(r, rec.get_tree(), rec);
                if (!expr) {
                    return false;
                }
                e = r.ReadorEOF();
                if (!e) {
                    return false;
                }
                if (!e->has_(":")) {
                    r.SetError(ErrorCode::expect_symbol, ":");
                    return false;
                }
                e = r.ConsumeReadorEOF();
                if (!e) {
                    return false;
                }
                if (!e->is_(TokenKind::identifiers)) {
                    return false;
                }
                tmp->to.push_back({expr, {e->to_string()}});
                r.Consume();
            }
            else if (e->has_("default")) {
                if (tmp->defaults.cargoname.size()) {
                    r.SetError(ErrorCode::multiple_default);
                    return false;
                }
                r.Consume();
                if (!e->has_(":")) {
                    r.SetError(ErrorCode::expect_symbol, ":");
                    return false;
                }
                e = r.ConsumeReadorEOF();
                if (!e) {
                    return false;
                }
                if (!e->is_(TokenKind::identifiers)) {
                    return false;
                }
                tmp->defaults.cargoname = e->to_string();
                r.Consume();
            }
            else {
                r.SetError(ErrorCode::unexpected_keyword);
                return false;
            }
        }
        cmd = tmp;
        return true;
    }

    bool parse_transfer(TokenReader& r, std::shared_ptr<Command>& cmd, Record& rec) {
        auto e = r.ReadorEOF();
        if (!e) {
            return false;
        }
        auto start = e;
        if (!e->is_(TokenKind::keyword) || !e->has_("transfer")) {
            r.SetError(ErrorCode::expect_keyword, "transfer");
            return false;
        }
        e = r.ConsumeReadorEOF();
        if (!e) {
            return false;
        }
        if (e->is_(TokenKind::keyword)) {
            if (e->has_("switch")) {
                return parse_switch(std::move(start), r, cmd, rec);
            }
            else if (e->has_("if")) {
                r.Consume();
                auto tmp = std::make_shared<TransferIf>(std::move(start));
                tmp->cond = binary(r, rec.get_tree(), rec);
                if (!tmp->cond) {
                    return false;
                }
                e = r.ReadorEOF();
                if (!e) {
                    return false;
                }
                if (!e->is_(TokenKind::identifiers)) {
                    r.SetError(ErrorCode::expect_id);
                    return false;
                }
                tmp->data.cargoname = e->to_string();
                r.Consume();
            }
            else {
                r.SetError(ErrorCode::unexpected_keyword);
                return false;
            }
        }
        else if (e->is_(TokenKind::identifiers)) {
            auto tmp = std::make_shared<TransferDirect>(std::move(start));
            tmp->data.cargoname = e->to_string();
            cmd = tmp;
            r.Consume();
        }
        else {
            r.SetError(ErrorCode::expect_id);
            return false;
        }
        return true;
    }

    template <class CmdType>
    bool parse_bind_or_test(TokenReader& r, std::shared_ptr<Command>& cmd, Record& rec, const char* bindtest) {
        auto e = r.ReadorEOF();
        if (!e) {
            return false;
        }
        if (!e->is_(TokenKind::keyword) || !e->has_(bindtest)) {
            r.SetError(ErrorCode::expect_keyword, bindtest);
            return false;
        }
        r.Consume();
        auto tmp = std::make_shared<CmdType>(std::move(e));
        tmp->expr = binary(r, rec.get_tree(), rec);
        if (!tmp->expr) {
            return false;
        }
        cmd = tmp;
        return true;
    }

    bool parse_assign(TokenReader& r, std::shared_ptr<Command>& cmd, Record& rec) {
        auto e = r.ReadorEOF();
        if (!e) {
            return false;
        }
        auto tmp = std::make_shared<AssignCommand>(std::move(e));
        if (!read_idname(r, tmp->target)) {
            return false;
        }
        e = r.ReadorEOF();
        if (!e) {
            return false;
        }
        if (!e->has_("=")) {
            return false;
        }
        r.Consume();
        tmp->expr = binary(r, rec.get_tree(), rec);
        if (!tmp->expr) {
            return false;
        }
        return true;
    }

    bool parse_command(TokenReader& r, std::shared_ptr<Command>& cmd, Record& rec);

    bool parse_if(TokenReader& r, std::shared_ptr<Command>& cmd, Record& rec) {
        EXPAND_MACRO(rec)
        auto e = r.ReadorEOF();
        if (!e) {
            return false;
        }
        if (!e->is_(TokenKind::keyword) || !e->has_("if")) {
            r.SetError(ErrorCode::expect_keyword, "if");
            return false;
        }
        r.Consume();
        EXPAND_MACRO(rec)
        auto tmp = std::make_shared<IfCommand>(std::move(e));
        auto read_block = [&](auto& cond) -> bool {
            EXPAND_MACRO(rec)
            e = r.ReadorEOF();
            if (!e) {
                return false;
            }
            if (!e->has_("{")) {
                return false;
            }
            r.Consume();
            while (true) {
                e = r.ReadorEOF();
                if (!e) {
                    return false;
                }
                if (e->has_("}")) {
                    r.Consume();
                    break;
                }
                std::shared_ptr<Command> tcmd;
                if (!parse_command(r, tcmd, rec)) {
                    return false;
                }
                cond->cmds.push_back(std::move(tcmd));
            }
            return true;
        };
        while (true) {
            auto cond = std::make_shared<IfCondition>();
            EXPAND_MACRO(rec)
            cond->expr = binary(r, rec.get_tree(), rec);
            if (!cond->expr) {
                return false;
            }
            if (!read_block(cond)) {
                return false;
            }
            tmp->ifs.push_back(std::move(cond));
            e = r.Read();
            if (e && e->is_(TokenKind::keyword)) {
                if (e->has_("elif")) {
                    r.Consume();
                    continue;
                }
                else if (e->has_("else")) {
                    r.Consume();
                    cond = std::make_shared<IfCondition>();
                    if (!read_block(cond)) {
                        return false;
                    }
                    tmp->ifs.push_back(std::move(cond));
                }
            }
            break;
        }
        cmd = tmp;
        return true;
    }

    bool parse_command(TokenReader& r, std::shared_ptr<Command>& cmd, Record& rec) {
        EXPAND_MACRO(rec)
        auto e = r.ReadorEOF();
        if (!e) {
            return false;
        }
        if (e->is_(TokenKind::keyword)) {
            if (e->has_("push")) {
                if (!parse_push_or_pop<PushCommand>(r, cmd, rec, "push")) {
                    return false;
                }
            }
            else if (e->has_("pop")) {
                if (!parse_push_or_pop<PopCommand>(r, cmd, rec, "pop")) {
                    return false;
                }
            }
            else if (e->has_("call")) {
                if (!parse_callcommand(r, cmd, rec)) {
                    return false;
                }
            }
            else if (e->has_("transfer")) {
                if (!parse_transfer(r, cmd, rec)) {
                    return false;
                }
            }
            else if (e->has_("bind")) {
                if (!parse_bind_or_test<BindCommand>(r, cmd, rec, "bind")) {
                    return false;
                }
            }
            else if (e->has_("test")) {
                if (!parse_bind_or_test<TestCommand>(r, cmd, rec, "test")) {
                    return false;
                }
            }
            else if (e->has_("if")) {
                if (!parse_if(r, cmd, rec)) {
                    return false;
                }
            }
            else {
                r.SetError(ErrorCode::unexpected_keyword);
                return false;
            }
        }
        else if (e->is_(TokenKind::identifiers)) {
            if (!parse_assign(r, cmd, rec)) {
                return false;
            }
        }
        else {
            r.SetError(ErrorCode::expect_keyword);
            return false;
        }
        return true;
    }
}  // namespace binred
