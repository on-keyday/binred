#include <fileio.h>
#include "parse/parse.h"
int main(int argc, char** argv) {
    binred::TokenReader red;
    commonlib2::Reader fin(commonlib2::FileReader("D:/MiniTools/binred/http2_frame.brd"));
    binred::ParseResult result;
    auto res = binred::parse_binred(fin, red, result);
}
