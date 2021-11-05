#include <fileio.h>
#include "parse/parse.h"
#include "output/cpp/cargo_to_struct.h"
#include "output/cpp/add_error_enum.h"
#include <coutwrapper.h>
#include <channel.h>
#include <thread>
#include <application/websocket.h>
#include <application/http.h>
#include <filesystem>
using namespace socklib;
using namespace commonlib2;

using Recv = commonlib2::RecvChan<std::shared_ptr<HttpServerConn>>;
using Send = commonlib2::ForkChan<std::shared_ptr<HttpServerConn>>;
auto& cout = commonlib2::cout_wrapper_s();

std::filesystem::path
normalize(const std::string& in) {
    std::u8string path = u8".";
    URLEncodingContext<std::u8string> enc;
    Reader(in).readwhile(path, url_decode, &enc);
    //cout << "debug:decoded path:" << (const char*)path.c_str() << "\n";
    std::filesystem::path pt(path);
    pt = pt.lexically_normal();
    return pt;
}

std::mutex roomlock;
std::map<std::string, ForkChan<std::string>> rooms;

std::string add_to_room(size_t& id, const std::string& name, const std::string& user, SendChan<std::string> rm) {
    const char* msg = "not joinable";
    roomlock.lock();
    if (auto found = rooms.find(name); found != rooms.end()) {
        found->second.subscribe(id, rm);
        found->second << "user " + user + " joined";
        msg = "joined";
    }
    roomlock.unlock();
    return msg;
}

std::string parse_command(const std::string& str, size_t& memberid, std::string& name, std::string& roomname) {
    auto cmd = commonlib2::split(str, " ");
    if (cmd[0] == "movroom") {
        if (cmd.size() != 2) {
            return "need room name";
        }
        std::string msg = "not movable to " + cmd[1];
        roomlock.lock();
        if (auto found = rooms.find(roomname); found != rooms.end()) {
        }
        roomlock.unlock();
    }
}

void handle_websocket(std::shared_ptr<WebSocketServerConn> conn) {
    auto [w, r] = commonlib2::make_chan<std::string>(100);
    size_t id = 0, timeoutcount = 0;
    bool pinged = false;
    auto e = add_to_room(id, "default", "guest", w);
    e += " to default room";
    std::string roomname = "default", user = "guest";
    conn->send_text(e.c_str());
    while (true) {
        if (conn->recvable() || Selecter::waitone(conn->borrow(), 0, 1)) {
            WsFrame frame;
            if (!conn->recv(frame)) {
                cout << conn->ipaddress() << ">closed";
                return;
            }
            if (frame.frame_type("text")) {
                auto resp = parse_command(frame.get_data(), id, user, roomname);
                conn->send_text(resp.c_str());
            }
        }
        else {
            timeoutcount++;
            if (pinged && timeoutcount > 300) {
                cout << conn->ipaddress() + "<closed\n";
                break;
            }
            else if (timeoutcount > 2000) {
                if (!conn->control(WsFType::ping)) {
                    cout << conn->ipaddress() + "<closed\n";
                    break;
                }
                cout << conn->ipaddress() + "<ping\n";
                pinged = true;
                timeoutcount = 0;
            }
            std::string data;
            if (auto e = r >> data) {
                conn->send_text(data.c_str());
            }
        }
    }
}

void handle_http(Recv r) {
    r.set_block(true);
    while (true) {
        std::shared_ptr<HttpServerConn> conn;
        if (r >> conn == commonlib2::ChanError::closed) {
            break;
        }
        if (!conn) {
            continue;
        }
        TimeoutContext timeout(60);
        SleeperContext ctx(&timeout);
        if (!conn->recv(&timeout)) {
            continue;
        }
        cout << "from " << conn->ipaddress() << ", by method";
        auto found = conn->request().find(":method");
        cout << found->second << ", request path ";
        cout << conn->path() + conn->query() << ", respond ";
        auto path = normalize(conn->path());
        if (path == "ws") {
            auto wsconn = WebSocket::default_hijack_server_proc(conn);
            if (!wsconn) {
                continue;
            }
            std::thread(handle_websocket, wsconn).detach();
            cout << 101 << "\n";
            continue;
        }
        conn->send(200, "OK", {{"Connection", "close"}}, "It Works", 8);
        cout << 200 << "\n";
    }
}

void binred_test() {
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
}

int main(int argc, char** argv) {
    auto [w, r] = commonlib2::make_chan<std::shared_ptr<HttpServerConn>>(100);
    for (auto i = 0; i < std::thread::hardware_concurrency(); i++) {
        std::thread(handle_http, r).detach();
    }
    Server sv;
    while (true) {
        auto accept = Http1::serve(sv, 8080);
        if (!accept) {
            w.close();
            break;
        }
        w << std::move(accept);
    }
}
