/*license*/
#pragma once

#include "../struct/read.h"
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
        e = r.ConsumeReadorEOF();
        if (!e) {
            return false;
        }
        auto tmp = std::make_shared<CmdType>();
        //auto tree = get_tree();
        tmp->numpop = binary(r, rec.get_tree(), rec);
        if (!tmp->numpop) {
            return false;
        }
        if (e->has_("$")) {
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
        auto tmp = std::make_shared<CallCommand>();
        if (tmp->call = parse_callexpr(r, rec); !tmp->call) {
            return false;
        }
        return true;
    }

    bool parse_switch(TokenReader& r, std::shared_ptr<Command>& cmd, Record& rec) {
        auto e = r.ReadorEOF();
        if (!e) {
            return false;
        }
        if (!e->is_(TokenKind::keyword) || !e->has_("switch")) {
            r.SetError(ErrorCode::expect_keyword, "switch");
            return false;
        }
        r.Consume();
        auto tmp = std::make_shared<TransferSwitch>();
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
                e = r.ReadorEOF();
                if (!e) {
                    return false;
                }
                if (!e->is_(TokenKind::identifiers)) {
                    return false;
                }
                tmp->to.push_back({expr, e->to_string()});
                r.Consume();
            }
            else if (e->has_("default")) {
                if (tmp->defaults.size()) {
                    r.SetError(ErrorCode::multiple_default);
                    return false;
                }
                r.Consume();
                if (!e->has_(":")) {
                    r.SetError(ErrorCode::expect_symbol, ":");
                    return false;
                }
                e = r.ReadorEOF();
                if (!e) {
                    return false;
                }
                if (!e->is_(TokenKind::identifiers)) {
                    return false;
                }
                tmp->defaults = e->to_string();
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
                return parse_switch(r, cmd, rec);
            }
            else if (e->has_("if")) {
                auto tmp = std::make_shared<TransferIf>();
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
                tmp->cargoname = e->to_string();
                r.Consume();
            }
            else {
                r.SetError(ErrorCode::unexpected_keyword);
                return false;
            }
        }
        else if (e->is_(TokenKind::identifiers)) {
            auto tmp = std::make_shared<TransferDirect>();
            tmp->cargoname = e->to_string();
            cmd = tmp;
            r.Consume();
        }
        else {
            r.SetError(ErrorCode::expect_id);
            return false;
        }
        return true;
    }

    bool parse_bind_or_test(TokenReader& r, std::vector<std::shared_ptr<Command>>& cmds, Record& rec, const char* bindtest) {
        auto e = r.ReadorEOF();
        if (!e) {
            return false;
        }
        if (!e->is_(TokenKind::keyword) || !e->has_(bindtest)) {
            r.SetError(ErrorCode::expect_keyword, bindtest);
            return false;
        }
        r.Consume();
    }

    bool parse_command(TokenReader& r, std::vector<std::shared_ptr<Command>>& cmds, Record& rec) {
        while (true) {
            auto e = r.ReadorEOF();
            if (!e) {
                return false;
            }
            std::shared_ptr<Command> cmd;
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
            }
            else if (e->is_(TokenKind::identifiers)) {
            }
        }
    }
}  // namespace binred
