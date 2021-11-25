/*
    commonlib - common utility library
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once

#include "tokendef.h"
#include "tokenparser.h"

#include "../serializer.h"
#include "../utfreader.h"

namespace PROJECT_NAME {
    namespace tokenparser {
        struct CodeLimit {  //number codeing like QUIC
            static constexpr size_t limit8 = 0x3f;
            static constexpr size_t limit16 = 0x3fff;
            static constexpr std::uint16_t mask16 = 0x4000;
            static constexpr std::uint8_t mask16_8 = 0x40;
            static constexpr size_t limit32 = 0x3fffffff;
            static constexpr std::uint32_t mask32 = 0x80000000;
            static constexpr std::uint8_t mask32_8 = 0x80;
            static constexpr size_t limit64 = 0x3fffffffffffffff;
            static constexpr std::uint64_t mask64 = 0xC000000000000000;
            static constexpr std::uint8_t mask64_8 = 0xC0;
        };

        struct BinaryIO {
            template <class Buf>
            static bool read_num(Deserializer<Buf>& target, size_t& size) {
                std::uint8_t e = target.base_reader().achar();
                std::uint8_t masked = e & CodeLimit::mask64_8;
                auto set_v = [&](auto& v) {
                    if (!target.read_ntoh(v)) {
                        return false;
                    }
                    size = v;
                    return true;
                };
                if (masked == CodeLimit::mask64_8) {
                    std::uint64_t v;
                    return set_v(v);
                }
                else if (masked == CodeLimit::mask32_8) {
                    std::uint32_t v;
                    return set_v(v);
                }
                else if (masked == CodeLimit::mask16_8) {
                    std::uint16_t v;
                    return set_v(v);
                }
                else {
                    std::uint8_t v;
                    return set_v(v);
                }
            }

            template <class Buf>
            static bool write_num(Serializer<Buf>& target, size_t size) {
                if (size <= CodeLimit::limit8) {
                    target.write_hton(std::uint8_t(size));
                }
                else if (size <= CodeLimit::limit16) {
                    std::uint16_t v = std::uint16_t(size) | CodeLimit::mask16;
                    target.write_hton(v);
                }
                else if (size <= CodeLimit::limit32) {
                    std::uint32_t v = std::uint32_t(size) | CodeLimit::mask32;
                    target.write_hton(v);
                }
                else if (size <= CodeLimit::limit64) {
                    std::uint64_t v = std::uint64_t(size) | CodeLimit::mask64;
                    target.write_hton(v);
                }
                else {
                    return false;
                }
                return true;
            }

            template <class Buf, class String>
            static bool write_string(Serializer<Buf>& target, String& str) {
                OptUTF8<String&> buf(str);
                if (buf.size() == 0) {
                    return false;
                }
                if (!write_num(target, buf.size())) {
                    return false;
                }
                for (size_t i = 0; i < buf.size(); i++) {
                    target.write(buf[i]);
                }
                return true;
            }

            template <class Buf, class String>
            static bool read_string(Deserializer<Buf>& target, String& str) {
                size_t size = 0;
                if (!read_num(target, size)) {
                    return false;
                }
                return target.read_byte(str, size);
            }
        };

        template <class Map>
        struct TokenWriteContext {
            Map keyword;
            Map symbol;
            size_t count = 0;
            Map id;
            template <class String>
            size_t getid(const String& v, bool& already) {
                if (auto found = id.find(v); found != id.end()) {
                    already = true;
                    return found->second;
                }
                count++;
                already = false;
                id[v] = count;
                return count;
            }
        };

        template <class Map>
        struct TokenReadContext {
            Map keyword;
            Map symbol;
            size_t count = 0;
            Map id;
            template <class String>
            size_t setstring(const String& v) {
                count++;
                id[count] = v;
                return count;
            }
        };

        struct TokenIO {
            template <class String, class Reg, class Map>
            static bool write_mapping(Serializer<String>& target, Registry<Reg>& reg, Map& map) {
                size_t count = 0;
                for (auto& v : reg.reg) {
                    if (map.insert({v, count}).second) {
                        count++;
                    }
                }
                if (!BinaryIO::write_num(target, map.size())) {
                    return false;
                }
                for (auto& v : map) {
                    if (!BinaryIO::write_string(target, v.first)) {
                        return false;
                    }
                }
                return true;
            }

            template <class String, class Reg, class Map>
            static bool read_mapping(Deserializer<String>& target, Registry<Reg>& reg, Map& map) {
                size_t count = 0;
                if (!BinaryIO::read_num(target, count)) {
                    return false;
                }
                for (size_t i = 0; i < count; i++) {
                    String v;
                    if (!BinaryIO::read_string(target, v)) {
                        return false;
                    }
                    reg.Register(v);
                    map.insert({i, v});
                }
                return true;
            }

            template <class Buf, class String, class Map>
            static bool read_token(Deserializer<Buf>& target, std::shared_ptr<Token<String>>& token, TokenReadContext<Map>& ctx) {
                TokenKind kind;
                size_t tmpsize = 0;
                if (!BinaryIO::read_num(target, tmpsize)) {
                    return false;
                }
                kind = TokenKind(tmpsize);
                switch (kind) {
                    case TokenKind::line: {
                        auto line = std::make_shared<Line<String>>();
                        //to type on editor easily
                        Line<String>* ptr = line.get();
                        if (!BinaryIO::read_num(target, tmpsize) {
                            return false;
                        }
                        ptr->linekind = LineKind(tmpsize);
                        if (!BinaryIO::read_num(target, tmpsize)) {
                            return false;
                        }
                        ptr->numline = tmpsize;
                        token=line;
                        return true;
                    }
                    case TokenKind::spaces: {
                        auto space = std::make_shared<Spaces<String>>();
                        //to type on editor easily
                        Spaces<String>* ptr = space.get();
                        if (!BinaryIO::read_num(target, tmpsize)) {
                            return false;
                        }
                        ptr->spchar = char32_t(tmpsize);
                        if (!BinaryIO::write_num(target, tmpsize)) {
                            return false;
                        }
                        ptr->numsp = tmpsize;
                        token = space;
                        return true;
                    }
                    case TokenKind::keyword:
                    case TokenKind::weak_keyword: {
                        auto keyword = std::make_shared<RegistryRead<String>>(kind);
                        //to type on editor easily
                        RegistryRead<String>* ptr = keyword.get();
                        if (!BinaryIO::read_num(target, tmpsize)) {
                            return false;
                        }
                        auto found = ctx.keyword.find(tmpsize);
                        if (found != ctx.keyword.end()) {
                            return false;
                        }
                        ptr->token = found->second;
                        token = keyword;
                        return true;
                    }
                    case TokenKind::symbols: {
                        auto symbol = std::make_shared<RegistryRead<String>>();
                        //to type on editor easily
                        RegistryRead<String>* ptr = symbol.get();
                        if (!BinaryIO::read_num(target, tmpsize)) {
                            return false;
                        }
                        auto found = ctx.symbol.find(tmpsize);
                        if (found != ctx.symbol.end()) {
                            return false;
                        }
                        ptr->token = found->second;
                        token = symbol;
                        return true;
                    }
                    case TokenKind::identifiers: {
                        auto id = std::make_shared<Identifier<String>>();
                        //to type on editor easily
                        Identifier<String>* ptr = id.get();
                        if (!BinaryIO::read_num(target, tmpsize)) {
                            return false;
                        }
                        if (tmpsize) {
                            auto found = ctx.id.find();
                            if (found != ctx.id.end()) {
                                return false;
                            }
                            ptr->id = found->second;
                        }
                        else {
                            if (!BinaryIO::read_string(target, ptr->id)) {
                                return false;
                            }
                            ctx.setstring(ptr->id);
                        }
                        token = id;
                        return true;
                    }
                    case TokenKind::comments: {
                        auto comment = std::make_shared<Comment<String>>();
                        Comment<String>* ptr = id.get();
                        if (!BinaryIO::read_num(target, tmpsize)) {
                            return false;
                        }
                        ptr->oneline = bool(tmpsize);
                        if (!BinaryIO::read_string(target, ptr->comments)) {
                            return false;
                        }
                        token = comment;
                        return true;
                    }
                    case TokenKind::root: {
                        token = std::make_shared<Token<String>>();
                        return true;
                    }
                    case TokenKind::unknown: {
                        return true;
                    }
                    default: {
                        return false;
                    }
                }
            }

            template <class Buf, class String, class Map>
            static bool write_token(Serializer<Buf>& target, Token<String>& token, TokenWriteContext<Map>& ctx) {
                auto kind = token.get_kind();
                if (!BinaryIO::write_num(target, size_t(kind))) {
                    return false;
                }
                switch (kind) {
                    case TokenKind::line: {
                        Line<String>* line = token.line();
                        if (!BinaryIO::write_num(target, size_t(line->get_linekind()))) {
                            return false;
                        }
                        if (!BinaryIO::write_num(target, size_t(line->get_linecount()))) {
                            return false;
                        }
                        return true;
                    }
                    case TokenKind::spaces: {
                        Spaces<String>* space = token.space();
                        if (!BinaryIO::write_num(target, size_t(space->get_spacechar()))) {
                            return false;
                        }
                        if (!BinaryIO::write_num(target, size_t(space->get_spacecount()))) {
                            return false;
                        }
                        return true;
                    }
                    case TokenKind::keyword:
                    case TokenKind::weak_keyword: {
                        RegistryRead<String>* keyword = token.registry();
                        auto found = ctx.keyword.find(keyword->get_token());
                        if (found == ctx.keyword.end()) {
                            return false;
                        }
                        if (!BinaryIO::write_num(target, size_t(found->second))) {
                            return false;
                        }
                        return true;
                    }
                    case TokenKind::symbols: {
                        RegistryRead<String>* symbol = token.registry();
                        auto found = ctx.symbol.find(symbol->get_token());
                        if (found == ctx.symbol.end()) {
                            return false;
                        }
                        if (!BinaryIO::write_num(target, size_t(found->second))) {
                            return false;
                        }
                        return true;
                    }
                    case TokenKind::identifiers: {
                        Identifier<String>* id = token.identifier();
                        bool already = false;
                        size_t idnum = ctx.getid(id->get_identifier(), already);
                        if (already) {
                            if (!BinaryIO::write_num(target, idnum)) {
                                return false;
                            }
                        }
                        else {
                            if (!BinaryIO::write_num(target, 0)) {
                                return false;
                            }
                            if (!BinaryIO::write_string(target, id->get_identifier())) {
                                return false;
                            }
                        }
                        return true;
                    }
                    case TokenKind::comments: {
                        Comment<String>* comment = token.comment();
                        if (!BinaryIO::write_num(target, std::uint8_t(comment->is_oneline()))) {
                            return false;
                        }
                        if (!BinaryIO::write_string(target, comment->get_comment())) {
                            return false;
                        }
                        return true;
                    }
                    case TokenKind::root: {
                        return true;
                    }
                    default: {
                        return false;
                    }
                }
            }
        };

        struct TokensIO {
            template <class Map, class Vector, class String, class Buf>
            static bool write_parsed(Serializer<Buf>& target, TokenParser<Vector, String>& p) {
                auto parsed = p.GetParsed();
                if (!parsed) {
                    return false;
                }
                TokenWriteContext<Map> ctx;
                target.write_byte("TkD0", 4);
                if (!TokenIO::write_mapping(target, p.keywords, ctx.keyword)) {
                    return false;
                }
                if (!TokenIO::write_mapping(target, p.symbols, ctx.symbol)) {
                    return false;
                }
                for (auto tok = parsed; tok; tok = tok->get_next()) {
                    if (!TokenIO::write_token(target, *tok, ctx)) {
                        return false;
                    }
                }
                if (!BinaryIO::write_num(target, size_t(TokenKind::unknown))) {
                    return false;
                }
                return true;
            }

            template <class Map, class Vector, class String, class Buf>
            static bool read_parsed(Deserializer<Buf>& target, TokenParser<Vector, String>& p) {
                TokenReadContext<Map> ctx;
                if (!target.base_reader().expect("TkD0")) {
                    return false;
                }
                if (!TokenIO::read_mapping(target, p.keywords, ctx.keyword)) {
                    return false;
                }
                if (!TokenIO::read_mapping(target, p.symbols, ctx.symbol)) {
                    return false;
                }
                std::shared_ptr<Token<String>> root;
                std::shared_ptr<Token<String>> prev;
                while (true) {
                    std::shared_ptr<Token<String>> tok;
                    if (!TokenIO::read_token(target, tok, ctx)) {
                        return false;
                    }
                    if (!tok) {
                        break;
                    }
                    if (!root) {
                        root = tok;
                        prev = tok;
                    }
                    else {
                        prev->force_set_next(tok);
                        prev = tok;
                    }
                }
                p.current = prev.get();
                p.roottoken.force_set_next(root);
                return true;
            }
        };
    }  // namespace tokenparser
}  // namespace PROJECT_NAME
