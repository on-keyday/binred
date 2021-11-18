/*
    binred - binary reader code generator
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#include <fileio.h>
#include "parse/parser/parse.h"
#include "output/cpp/cargo_to_struct.h"
#include "output/cpp/alias_to_enum.h"
#include "output/cpp/add_error_enum.h"
#include <iostream>
#include <fstream>
#include <optmap.h>
#include <subcommand.h>
#include <functional>

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

constexpr std::pair<commonlib2::U8MiniBuffer, char32_t> utf_convert(char32_t c) {
    commonlib2::U8MiniBuffer minbuf;
    commonlib2::make_utf8_from_utf32(c, minbuf);
    auto restore_c = commonlib2::make_utf32_from_utf8(minbuf, minbuf.size());
    return {minbuf, restore_c};
}

int main(int argc, char** argv) {
    commonlib2::SubCommand cmd(std::string("root"));
    commonlib2::SubCmdDispatch disp(std::string("root"), [](auto& r) {
        std::cout << "unhandled command: " << r.get_current()->get_cmdname();
    });
    disp.set_subcommand(
        "hello",
        {},
        [](auto&) {
            return 0;
        });
    cmd.set_option({
        {"input", {'i'}, "set input files", 1, false, true},
        {"language", {'l'}, "set output language (cpp)", 1, false, true},
        {"output", {'o'}, "set output file", 1, false, true},
    });
    cmd.set_subcommand("get", {{"where", {'w'}, "set where fetch from", 1, false, true}});
    decltype(cmd)::result_t result;
    if (auto err = cmd.parse_opt(argc, argv, result, commonlib2::OptOption::getopt_mode,
                                 [](auto& op, bool on_error) {
                                     return true;
                                 });
        !err) {
    }
    binred_test();
    constexpr std::uint32_t uv = U'9';
    constexpr auto e = utf_convert(uv);
    constexpr auto check = e.second == uv;
}
