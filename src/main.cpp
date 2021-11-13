#include <fileio.h>
#include "parse/parser/parse.h"
#include "output/cpp/cargo_to_struct.h"
#include "output/cpp/alias_to_enum.h"
#include "output/cpp/add_error_enum.h"
#include <iostream>
#include <fstream>

void binred_test() {
    binred::TokenReader red;
    binred::ParseResult result;
    binred::Record record;
    {
        commonlib2::Reader fin(commonlib2::FileReader("D:/MiniTools/binred/http2_frame.brd"));
        binred::parse_binred(fin, red, record, result);
    }
    binred::OutContext ctx;
    for (auto& a : record.aliases) {
        binred::AliasToCppEnum::convert(ctx, *a.second);
    }
    for (auto& c : result) {
        if (c->type == binred::ElementType::cargo) {
            auto cg = static_cast<binred::Cargo*>(&*c);
            binred::CargoToCppStruct::convert(ctx, *cg, record);
        }
    }
    {
        std::ofstream fs("D:/MiniTools/binred/generated/test.hpp");
        std::cout << ctx.buffer;
        fs << "/*license*/\n#pragma once\n#include<cstdint>\n#include<string>\n";
        fs << binred::error_enum_class(ctx);
        fs << ctx.buffer;
    }
}

constexpr auto testf(char32_t c) {
    commonlib2::U8MiniBuffer minbuf;
    commonlib2::make_utf8_from_utf32(c + 1, minbuf);
    return minbuf;
}

constexpr auto testvalue(char32_t c) {
    auto result = testf(c);
    auto a = result[0] + result[1] + result[2] + result[3];
    if (a >= 1000) {
        throw "boo";
    }
    return std::tuple{result, a};
}

int main(int argc, char** argv) {
    binred_test();
    constexpr auto str = u8"\x21";
    constexpr auto e = testvalue(commonlib2::make_utf32_from_utf8(str, sizeof(str) - 1));
}
