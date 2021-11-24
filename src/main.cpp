/*
    binred - binary I/O code generator
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
#include "syntax/syntax.h"
#include "syntax_rule/set_by_syntax.h"

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
    disp.set_helpstr("binary I/O generator");
    disp.set_usage("binred [<option>] <subcommand>");
    disp.set_callback([](decltype(disp)::result_t& r) {
        if (r.get_current()->get_cmdname() == "binred") {
            if (auto arg = r.get_layer(0)->has_(":arg")) {
                std::cout << r.errorln(arg->arg()->at(0) + ": no such subcommand exists\ntry `binred help` for more info");
            }
            else {
                std::cout << r.errorln("need subcommand\ntry `binred help` for more info");
            }
            return -1;
        }
        else {
            std::cout << r.errorln("command not implemented");
        }
        return 0;
    });
    disp.set_option({
        {"process", {'p'}, "set maximum thread count (1-" + std::to_string(std::thread::hardware_concurrency()) + ")", 1, true},
    });
    disp.set_subcommand(
            "help", "show command help",
            {},
            [](decltype(disp)::result_t& r) {
                auto arg = r.get_layer("help")->has_(":arg");
                auto base = r.get_current()->get_parent();
                if (!arg) {
                    std::cout << r.help(base, "usage:", "subcommand:", 3);
                    return 0;
                }
                auto& key = arg->arg()->at(0);
                auto c = base->get_subcmd(key);
                if (!c) {
                    std::cout << r.errorln(key + ": no such subcommand exists");
                    return 1;
                }
                std::cout << r.help(c, "usage:", "subcommand:", 3);
                return 0;
            })
        ->set_usage("binred help <command>");
    disp.set_subcommand(
            "hello", "say hello",
            {},
            [](decltype(disp)::result_t& r) {
                std::cout << r.errorln("Hello!");
                return std::pair{0, false};
            })
        ->set_usage("binred hello");
    disp.set_subcommand(
            "build", "translate binred to other language",
            {
                {"input", {'i'}, "set input files", 1, false, true},
                {"language", {'l'}, "set output language (cpp)", 1, false, true},
                {"output", {'o'}, "set output file", 1, false, true},
            })
        ->set_usage("binred build [<options>]");
    disp.set_subcommand(
            "get", "get package from the Internet",
            {
                {"where", {'w'}, "set where fetch from", 1, false, true},
            })
        ->set_usage("binred get [<options>] <url>");

    std::string msg;
    if (auto err = disp.run(argc, argv, commonlib2::OptOption::getopt_mode,
                            [&](auto& op, bool on_error) {
                                if (on_error) {
                                    msg = "-" + op + ": ";
                                }
                                else {
                                    std::cout << "binred: warning: unknown option -" + op + " ignored\n";
                                }
                                return true;
                            });
        !err.first) {
        std::cout << "binred: error: " << msg << commonlib2::error_message(err.first);
    }
    binred::syntax::SyntaxCompiler syntaxc;
    using File = commonlib2::Reader<commonlib2::FileReader>;
    {
        File syntaxfile(commonlib2::FileReader("src/syntax_file/syntax.txt"));
        if (!syntaxc.make_parser(syntaxfile)) {
            std::cout << "error: " << syntaxc.error();
        }
    }
    binred::Stmts stmts;
    syntaxc.callback() = [&](const binred::syntax::MatchingContext& c) {
        if (!c.is_invisible_type()) {
            std::cout << c.current() << ":" << type_str(c.get_type()) << ":" << c.get_token() << "\n";
        }
        return stmts(c);
    };
    {
        File testfile(commonlib2::FileReader("src/syntax_file/test_syntax2.txt"));
        if (!syntaxc.parse(testfile)) {
            std::cout << "error:\n"
                      << syntaxc.error();
        }
    }

    //binred_test();
}
