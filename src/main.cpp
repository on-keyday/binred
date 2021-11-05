#include <fileio.h>
#include "parse/parse.h"
#include "output/cpp/cargo_to_struct.h"
#include "output/cpp/add_error_enum.h"
#include <iostream>
#include <fstream>
#include <channel.h>
#include <thread>
using Recv = commonlib2::RecvChan<int>;
void test_thread(Recv r) {
}

int main(int argc, char** argv) {
    binred::TokenReader red;

    binred::ParseResult result;
    {
        commonlib2::Reader fin(commonlib2::FileReader("D:/MiniTools/binred/http2_frame.brd"));
        binred::parse_binred(fin, red, result);
    }
    binred::OutContext ctx;
    auto c = static_cast<binred::Cargo*>(&*result[0]);
    binred::CargoToCppStruct::convert(ctx, *c);
    std::ofstream fs("D:/MiniTools/binred/generated/test.hpp");
    std::cout << ctx.buffer;
    fs << "/*license*/\n#pragma once\n#include<cstdint>\n#include<string>\n";
    fs << binred::error_enum_class(ctx);
    fs << ctx.buffer;
    auto [w, r] = commonlib2::make_chan<int>();
    std::thread(test_thread, r).detach();
}
