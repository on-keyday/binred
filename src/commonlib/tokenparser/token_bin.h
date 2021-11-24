/*
    commonlib - common utility library
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once

#include "tokendef.h"

#include "../serializer.h"

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

        template <class String>
        struct BinaryIO {
            static bool read_num(Deserializer<String>& target, size_t& size) {
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

            static bool write_num(Serializer<String>& target, size_t size) {
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
                return false;
            }
        };
    }  // namespace tokenparser
}  // namespace PROJECT_NAME
