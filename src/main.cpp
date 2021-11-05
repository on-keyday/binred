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
    r.set_block(true);
    bool block = true;
    while (true) {
        int data = 0;
        if (auto e = r >> data) {
            std::cout << data << "\n";
            Sleep(1);
            block = !block;
            r.set_block(block);
        }
        else if (e == commonlib2::ChanError::empty) {
            std::cout << "empty\n";
        }
        else if (e == commonlib2::ChanError::closed) {
            std::cout << "closed\n";
            break;
        }
    }
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
    {
        std::ofstream fs("D:/MiniTools/binred/generated/test.hpp");
        std::cout << ctx.buffer;
        fs << "/*license*/\n#pragma once\n#include<cstdint>\n#include<string>\n";
        fs << binred::error_enum_class(ctx);
        fs << ctx.buffer;
    }
    auto [w, r] = commonlib2::make_chan<int>();
    commonlib2::ForkChan<int> fork;
    fork << 92;
    std::thread(test_thread, std::move(r)).detach();
    size_t count = 0;
    while (w << count) {
        if (count >= 1000) {
            w.close();
            break;
        }
        std::cout << "sleeping delta...\n";
        Sleep(1000);
        count++;
    }
    Sleep(10);
}
