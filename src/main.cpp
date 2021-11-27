/*
    binred - binary I/O code generator
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/


#include "parse/parser/parse.h"
#include "output/cpp/cargo_to_struct.h"
#include "output/cpp/alias_to_enum.h"
#include "output/cpp/add_error_enum.h"
#include <iostream>
#include <fstream>
#include <optmap.h>
#include <subcommand.h>
#include <syntax/syntax.h>
#include "syntax_rule/set_by_syntax.h"
#include <syntax/syntax_bin.h>
#include <pack/utf8io.h>
namespace cl2 = commonlib2;

auto& cout = commonlib2::cout_wrapper();

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
        cout << ctx.buffer;
        fs << "/*license*/\n#pragma once\n#include<cstdint>\n#include<string>\n";
        fs << binred::cpp::error_enum_class(ctx);
        fs << ctx.buffer;
    }
}

void test_syntax(cl2::SubCmdDispatch<>::result_t&) {
    binred::syntax::SyntaxCompiler syntaxc;
    using File = commonlib2::Reader<commonlib2::FileReader>;
    {
        File syntaxfile(commonlib2::FileReader("src/syntax_file/syntax.txt"));
        if (!syntaxc.make_parser(syntaxfile)) {
            cout << "error: " << syntaxc.error();
        }
    }
    binred::Stmts stmts;
    syntaxc.callback() = [&](const binred::syntax::MatchingContext& c) {
        if (!c.is_invisible_type()) {
            cout << c.current() << ":" << type_str(c.get_type()) << ":" << c.get_token() << "\n";
        }
        return stmts(c);
    };
    {
        File testfile(commonlib2::FileReader("src/syntax_file/test_syntax2.txt"));
        if (!syntaxc.parse(testfile)) {
            cout << "error:\n"
                 << syntaxc.error();
        }
    }
    /*
    commonlib2::Serializer<std::string> target;

    commonlib2::Deserializer<std::string&> target2(target.get());

    auto result = commonlib2::syntax::SyntaxIO::save(target, syntaxc, true);

    commonlib2::syntax::SyntaxCompiler stxc;
    result = commonlib2::syntax::SyntaxIO::load(target2, stxc);
    std::ofstream("src/syntax_file/parsed.dat", std::ios_base::binary) << target.get();
    */
}

int main(int argc, char** argv) {
    commonlib2::IOWrapper::Init();
    commonlib2::ArgChange _(argc, argv);
    commonlib2::SubCmdDispatch disp(std::string("binred"));
    disp.set_helpstr("binary I/O generator");
    disp.set_usage("binred [<option>] <subcommand>");
    disp.set_callback([](decltype(disp)::result_t& r) {
        if (r.get_current()->get_cmdname() == "binred") {
            if (auto arg = r.get_layer(0)->has_(":arg")) {
                cout << r.fmtln(arg->arg()->at(0) + ": no such subcommand exists\ntry `binred help` for more info");
            }
            else {
                cout << r.fmtln("need subcommand\ntry `binred help` for more info");
            }
            return -1;
        }
        else {
            cout << r.fmtln("command not implemented");
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
                    cout << r.help(base, "usage:", "subcommand:", 3);
                    return 0;
                }
                std::string v;
                const decltype(disp)* c = base;
                for (auto& key : *arg->arg()) {
                    v += key + ": ";
                    auto tmp = c->get_subcmd(key);
                    if (!tmp) {
                        cout << r.fmtln(v + "no such subcommand exists");
                        return 1;
                    }
                    c = tmp;
                }
                cout << r.help(c, "usage:", "subcommand:", 3);
                return 0;
            })
        ->set_usage("binred help <command>");
    disp.set_subcommand(
            "hello", "say hello",
            {},
            [](decltype(disp)::result_t& r) {
                cout << r.fmtln("Hello!");
                return 0;
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
    disp.set_subcommand(
            "syntaxc", "compile syntax file",
            {
                {"input-file", {'i'}, "set input file (required)", 1, true},
                {"output-file", {'o'}, "set output file (required)", 1, true},
                {"minimum", {'m'}, "compress tokens (level 0-3)", 1, true},
            },
            [](decltype(disp)::result_t& result) {
                auto layer = result.get_layer("syntaxc");
                auto args = layer->has_("input-file");
                if (!args) {
                    cout << result.fmt("need input file name");
                    return 1;
                }
                binred::syntax::SyntaxCompiler syntaxc, testc;
                using File = commonlib2::Reader<commonlib2::FileReader>;
                {
                    auto& input = args->arg()->at(0);
                    File syntaxfile(commonlib2::FileReader(cl2::ToPath(input).c_str()));
                    if (!syntaxfile.ref().is_open()) {
                        cout << result.fmt("file " + input + " couldn't open");
                        return -1;
                    }
                    if (!syntaxc.make_parser(syntaxfile)) {
                        cout << result.fmt(syntaxc.error());
                        return -1;
                    }
                }
                {
                    args = layer->has_("output-file");
                    if (!args) {
                        cout << result.fmt("need output file name");
                        return 1;
                    }
                    auto& output = args->arg()->at(0);
                    commonlib2::Serializer<commonlib2::FileWriter> w(commonlib2::FileWriter(cl2::ToPath(output).c_str()));
                    if (!w.get().is_open()) {
                        cout << result.fmt("file " + output + " couldn't open");
                        return -1;
                    }
                    int compress = 0;
                    args = layer->has_("minimum");
                    if (args) {
                        cl2::Reader(args->arg()->at(0)) >> compress;
                    }
                    commonlib2::Serializer<std::string> test;
                    if (!commonlib2::syntax::SyntaxIO::save(test, syntaxc, compress)) {
                        cout << result.fmt("failed to write syntax to " + output);
                        return -1;
                    }
                    commonlib2::Deserializer<std::string&> test2(test.get());
                    auto res = commonlib2::syntax::SyntaxIO::load(test2, testc);
                    if (res) {
                        cout << "compressed image:\n";
                        for (auto tok = testc.get_rawparser().GetParsed(); tok; tok = tok->get_next()) {
                            cout << tok->to_string();
                        }
                        cout << "\n";
                    }
                    w.write_byte(test.get());
                    cout << result.fmt("operation succeeded. result saved to " + output);
                }
                return 0;
            })
        ->set_usage("binred syntaxc <option>");
    disp.set_subcommand("debug", "for debug", {}, test_syntax)
        ->set_usage("binred debug");
    std::string msg;
    auto err = disp.run(argc, argv, commonlib2::OptOption::getopt_mode,
                        [&](auto& op, bool on_error) {
                            if (on_error) {
                                msg = "-" + op + ": ";
                            }
                            else {
                                cout << "binred: warning: unknown option -" + op + " ignored\n";
                            }
                            return true;
                        });
    if (!err.first) {
        cout << "binred: error: " << msg << commonlib2::error_message(err.first);
    }
    return err.second;
}
