#include <fileio.h>
#include "parse/parse.h"
#include "output/cpp/cargo_to_struct.h"
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
    auto c = static_cast<binred::Cargo*>(&*result[0]);
    binred::CargoToCppStruct::convert(ctx, *c);
    {
        std::ofstream fs("D:/MiniTools/binred/generated/test.hpp");
        std::cout << ctx.buffer;
        fs << "/*license*/\n#pragma once\n#include<cstdint>\n#include<string>\n";
        fs << binred::error_enum_class(ctx);
        fs << ctx.buffer;
    }
}

int main(int argc, char** argv) {
}
