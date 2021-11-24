/*
    commonlib - common utility library
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once

#include "tokendef.h"

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
            size_t getid(const String& v) {
                if (auto found = id.find(v); found != id.end()) {
                    return found->second;
                }
                id[v] = count;
                count++;
                return count - 1;
            }
        };

        struct TokenIO {
            template <class Reg, class Map>
            void set_mapping(Registry<Reg>& reg, Map& map) {
                size_t count = 0;
                for (auto& v : reg.reg) {
                    map.insert({v, count});
                    count++;
                }
            }

            template <class Buf, class String, class Map>
            bool read_token(Deserializer<Buf>& target, Token<String>& token, TokenWriteContext<Map>& ctx) {
                TokenKind kind;
                size_t tmpsize = 0;
                if (!BinaryIO::read_num(target, tmpsize)) {
                    return false;
                }
                kind = TokenKind(tmpsize);
                switch (kind) {
                    case TokenKind::line: {
                        std::shared_ptr<Line<String>> line = std::make_shared<Line<String>>();
                        //to type on editor easily
                        Line<String>* ptr = line.get();
                        if (!BinaryIO::read_num(target, tmpsize) {
                            return false;
                        }
                        ptr->linekind = LineKind(tmpsize);
                        if (!BinaryIO::read_num(target, tmpsize) {
                            return false;
                        }
                        ptr->numline = tmpsize;
                        token=line;
                        return true;
                    }
                }
            }

            template <class Buf, class String, class Map>
            bool write_token(Serializer<Buf>& target, Token<String>& token, TokenWriteContext<Map>& ctx) {
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
                        if (!BinaryIO::write_num(target, size_t(line->get_spacecount()))) {
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
                        if(!BinaryIO::write_num(target,ctx.getid(id->get_identifier())){
                            return false;
                        }
                        return true;
                    }
                    case TokenKind::comments: {
                        Comment<String>* comment = token.comment();
                        if (!BinaryIO::write_string(target, comment->get_comment())) {
                            return false;
                        }
                        return true;
                    }
                    case TokenKind::root:
                    case TokenKind::identifiers: {
                        return true;
                    }
                    default: {
                        return false;
                    }
                }
            }
        };
    }  // namespace tokenparser
}  // namespace PROJECT_NAME
