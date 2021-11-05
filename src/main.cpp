#include <fileio.h>
#include "parse/parse.h"
#include "output/cpp/cargo_to_struct.h"
#include "output/cpp/add_error_enum.h"
#include <iostream>
#include <fstream>
#include <channel.h>
#include <thread>
#include <sstream>
using Recv = commonlib2::RecvChan<int>;
using Send = commonlib2::ForkChan<int>;
void test_thread(Recv r, Send w) {
    r.set_block(true);
    while (true) {
        int data = 0;
        if (auto e = r >> data) {
            if (data != 0) {
                std::stringstream ss;
                ss << std::this_thread::get_id() << ":" << data << "\n";
                std::cout << ss.str();
            }
            Sleep(1);
            w << std::move(data);
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
    size_t id = 0;
    auto fork = commonlib2::make_forkchan<int>();
    for (auto i = 0; i < 12; i++) {
        auto [w, r] = commonlib2::make_chan<int>(5, commonlib2::ChanDisposeFlag::remove_back);
        size_t id;
        fork.subscribe(id, w);
        std::thread(test_thread, r, fork).detach();
    }
    size_t count = 0;
    while (fork << count) {
        if (count >= 1000) {
            fork.close();
            break;
        }
        std::cout << "sleeping delta...\n";
        //Sleep(1000);
        Sleep(1);
        count++;
    }
    Sleep(1000);
}
