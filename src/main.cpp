#include <fileio.h>
#include "parse/parser/parse.h"
#include "output/cpp/cargo_to_struct.h"
#include "output/cpp/alias_to_enum.h"
#include "output/cpp/add_error_enum.h"
#include <iostream>
#include <fstream>
#include <learnstd.h>

void binred_test() {
    binred::TokenReader red;
    binred::ParseResult result;
    binred::Record record;
    {
        commonlib2::Reader fin(commonlib2::FileReader("D:/MiniTools/binred/http2_frame.brd"));
        binred::parse_binred(fin, red, record, result);
    }
    binred::cpp::CppOutContext ctx;
    for (auto& a : record.aliases) {
        binred::cpp::AliasToCppEnum::convert(ctx, *a.second);
    }
    for (auto& c : result) {
        if (c->type == binred::ElementType::cargo) {
            auto cg = binred::castptr<binred::Cargo>(c);
            binred::cpp::CargoToCppStruct::convert(ctx, *cg, record);
        }
    }
    {
        std::ofstream fs("D:/MiniTools/binred/generated/test.hpp");
        std::cout << ctx.buffer;
        fs << "/*license*/\n#pragma once\n#include<cstdint>\n#include<string>\n";
        fs << binred::cpp::error_enum_class(ctx);
        fs << ctx.buffer;
    }
}

constexpr auto testf(char32_t c) {
    commonlib2::U8MiniBuffer minbuf;
    commonlib2::make_utf8_from_utf32(c, minbuf);
    return minbuf;
}

constexpr auto testvalue(char32_t c) {
    auto result = testf(c);
    auto a = result[0] + result[1] + result[2] + result[3];
    return std::pair{result, a};
}

constexpr auto testvalues(size_t begin, size_t end = 0) {
    if (end == 0) {
        end = end + 10000;
    }
    for (auto i = begin; i < end; i++) {
        auto res = testvalue(i);
        if (res.second >= 900) {
            return res;
        }
    }
    return testvalue(0);
}

int main(int argc, char** argv) {
    binred_test();
}
