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
            static constexpr size_t limit32 = 0x3fffffff;
            static constexpr std::uint32_t mask32 = 0x80000000;
            static constexpr size_t limit64 = 0x3fffffffffffffff;
            static constexpr std::uint64_t mask64 = 0xC000000000000000;
        };

        template <class String>
        struct BinaryWriter {
            Serializer<String> target;

            bool write_num(size_t size) {
                if (size <= CodeLimit::limit8) {
                    target.write_hton(std::uint8_t(size));
                }
                else if (size <= CodeLimit::limit16) {
                    auto v = std::uint16_t(size);
                    target.write_hton();
                }
                target.write_hton();
            }
        };
    }  // namespace tokenparser
}  // namespace PROJECT_NAME
