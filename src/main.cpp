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

int main(int argc, char** argv) {
    commonlib2::SubCmdDispatch disp(std::string("binred"));
    disp.set_callback([](decltype(disp)::result_t& r) {
        std::cout << r.errorln("unhandled command");
    });
    disp.set_subcommand(
        "hello",
        {},
        [](auto&) {

        });
    disp.set_option({
        {"input", {'i'}, "set input files", 1, false, true},
        {"language", {'l'}, "set output language (cpp)", 1, false, true},
        {"output", {'o'}, "set output file", 1, false, true},
    });
    disp.set_subcommand("get", {{"where", {'w'}, "set where fetch from", 1, false, true}});

    if (auto err = disp.run(argc, argv, commonlib2::OptOption::getopt_mode,
                            [](auto& op, bool on_error) {
                                return true;
                            });
        !err.first) {
    }
    binred_test();
}
