/*license*/
#pragma once
#include "element.h"
#include <vector>
#include <string>
#include <map>
#include "../parser/parser.h"
#include <regex>

namespace binred {
    struct Macro : Element {
        Macro(std::shared_ptr<token_t>&& tok)
            : Element(std::move(tok), ElementType::macro) {}
        std::string name;
        std::vector<std::string> args;
        std::string expand;
    };

    struct MacroExpander {
        std::map<std::string, std::shared_ptr<Macro>> macros;
        bool add(const std::string& name, std::shared_ptr<Macro> m) {
            if (!m) {
                return false;
            }
            return macros.insert({name, m}).second;
        }

        std::shared_ptr<Macro> find(const std::string& name) {
            if (auto found = macros.find(name); found != macros.end()) {
                return found->second;
            }
            return nullptr;
        }

        bool read_bracket(TokenReader& r, std::shared_ptr<Macro>& found, std::vector<std::string>& vec) {
            auto e = r.ConsumeReadorEOF();
            if (!e) {
                return false;
            }
            if (!e->has_("(")) {
                r.SetError(ErrorCode::expect_symbol, "(");
                return false;
            }
            r.Consume();
            size_t count = found->args.size();
            while (count != 0) {
                r.Read();
                std::string str;
                while (true) {
                    auto a = r.GetorEOF();
                    if (!a) {
                        return false;
                    }
                    if (count == 1) {
                        if (a->has_(")")) {
                            break;
                        }
                        else if (a->has_(",") || a->has_("!")) {
                            r.SetError(ErrorCode::unexpected_symbol, ", or !");
                            return false;
                        }
                    }
                    else {
                        if (a->has_(",")) {
                            r.Consume();
                            break;
                        }
                        else if (a->has_(")") || a->has_("!")) {
                            r.SetError(ErrorCode::unexpected_symbol, ") or !");
                            return false;
                        }
                    }
                    if (a->is_(TokenKind::comments) || a->has_("/*") || a->has_("*/") || a->has_("*/")) {
                        continue;
                    }
                    str.append(a->to_string());
                    r.Consume();
                }
                vec.push_back(std::move(str));
                count--;
            }
            e = r.ReadorEOF();
            if (!e) {
                return false;
            }
            if (!e->has_(")")) {
                return false;
            }
            return true;
        }

        bool expand(TokenReader& r) {
            auto e = r.Read();
            if (!e) {
                return true;
            }
            if (e->has_("!")) {
                auto begin = e;
                r.Consume();
                auto name = r.GetorEOF();
                if (!name) {
                    return false;
                }
                auto id = name->identifier();
                if (!id) {
                    r.SetError(ErrorCode::expect_id);
                    return false;
                }
                auto found = find(id->get_identifier());
                if (!found) {
                    r.SetError(ErrorCode::undefined_macro);
                    return false;
                }
                auto expand = found->expand;
                if (found->args.size() != 0) {
                    std::vector<std::string> vec;
                    if (!read_bracket(r, found, vec)) {
                        return false;
                    }
                    for (size_t i = 0; i < found->args.size(); i++) {
                        expand = std::regex_replace(expand, std::regex(found->args[i]), vec[i]);
                    }
                }
                TokenGetter gt;
                commonlib2::Reader<std::string&> tr(expand);
                if (!gt.parse(tr)) {
                    r.SetError(ErrorCode::invalid_comment);
                    return false;
                }
                auto con1 = begin->get_prev();
                auto con2 = r.current->get_next();
                begin->remove();
                r.current->remove();
                con1->force_set_next(gt.parser.GetParsed()->get_next());
                for (auto i = con1->get_next();; i = i->get_next()) {
                    if (!i->get_next()) {
                        i->force_set_next(con2);
                        break;
                    }
                }
                r.current = con1->get_next();
            }
            return true;
        }
    };
#define EXPAND_MACRO(mep) \
    if (!mep.expand(r)) { \
        return 0;         \
    }
}  // namespace binred
