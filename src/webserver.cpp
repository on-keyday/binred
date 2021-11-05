#include <application/websocket.h>
#include <fileio.h>
#include <coutwrapper.h>
#include <channel.h>
#include <thread>
#include <filesystem>
#include <direct.h>
using namespace socklib;
using namespace commonlib2;

using Recv = commonlib2::RecvChan<std::shared_ptr<HttpServerConn>>;
using Send = commonlib2::ForkChan<std::shared_ptr<HttpServerConn>>;
auto& cout = commonlib2::cout_wrapper_s();
auto& cin = commonlib2::cin_wrapper();

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

struct WsSession {
    RecvChan<std::string> r;
    SendChan<std::string> w;
    WsSession(RecvChan<std::string> r, SendChan<std::string> w)
        : r(r), w(w) {}
    size_t id = 0, timeoutcount = 0;
    bool pinged = false;
    std::string roomname = "default", user = "guest";
};

std::string add_to_room(size_t& id, const std::string& name, const std::string& user, SendChan<std::string> rm) {
    std::string msg = "not joinable to" + name;
    if (auto found = rooms.find(name); found != rooms.end()) {
        found->second.subscribe(id, rm);
        found->second << "user " + user + " joined";
        msg = "joined to " + name;
    }
    return msg;
}

std::string make_room(const std::string& name) {
    auto room = make_forkchan<std::string>();
    if (!rooms.insert({name, room}).second) {
        return "can't make room " + name;
    }
    return "room " + name + " created";
}

void leave_room(bool nocomment, size_t id, const std::string& roomname, const std::string& user) {
    if (auto found = rooms.find(roomname); found != rooms.end()) {
        if (!nocomment) {
            found->second << "leave " + user + " from " + roomname;
        }
        found->second.remove(id);
        if (!found->second.size() && found->first != "default") {
            rooms.erase(found->first);
        }
    }
}

std::string parse_command(const std::string& str, WsSession& se) {
    auto cmd = commonlib2::split(str, " ");
    if (!cmd.size()) {
        return "need any command";
    }
    if (cmd[0] == "movroom") {
        if (cmd.size() != 2) {
            return "need room name";
        }
        std::string msg = "not movable to " + cmd[1];
        roomlock.lock();
        leave_room(false, se.id, se.roomname, se.user);
        make_room(cmd[1]);
        add_to_room(se.id, cmd[1], se.user, se.w);
        roomlock.unlock();
        se.roomname = cmd[1];
        return "moved to " + cmd[1];
    }
    else if (cmd[0] == "close") {
        return "close";
    }
    else if (cmd[0] == "cast") {
        commonlib2::Reader r(str);
        r.expect("cast");
        auto tosend = str.substr(r.readpos());
        tosend = se.user + ">" + tosend;
        roomlock.lock();
        if (auto found = rooms.find(se.roomname); found != rooms.end()) {
            found->second << std::move(tosend);
            tosend = "";
        }
        else {
            tosend = "can't send";
        }
        roomlock.unlock();
        return tosend;
    }
    return "no such command:" + cmd[1];
}

void handle_websocket(std::shared_ptr<WebSocketServerConn> conn) {
    cout << conn->ipaddress() << ">connect\n";
    auto [w, r] = commonlib2::make_chan<std::string>(100);
    WsSession se(r, w);
    roomlock.lock();
    auto e = add_to_room(se.id, "default", "guest", se.w);
    roomlock.unlock();
    cout << conn->ipaddress() << "<data\n";
    conn->send_text(e.c_str());
    while (true) {
        if (conn->recvable() || Selecter::waitone(conn->borrow(), 0, 1)) {
            WsFrame frame;
            if (!conn->recv(frame)) {
                cout << conn->ipaddress() << ">closed\n";
                return;
            }
            if (frame.frame_type("text") || frame.frame_type("binary")) {
                cout << conn->ipaddress() << ">data\n";
                cout << frame.get_data() << "\n";
                auto resp = parse_command(frame.get_data(), se);
                cout << conn->ipaddress() << "<data\n";
                cout << resp;
                conn->send_text(resp.c_str());
                if (resp == "close") {
                    cout << conn->ipaddress() << "<close\n";
                    conn->close();
                    break;
                }
            }
        }
        else {
            se.timeoutcount++;
            if (se.pinged && se.timeoutcount > 300) {
                cout << conn->ipaddress() + "<closed\n";
                break;
            }
            else if (se.timeoutcount > 2000) {
                if (!conn->control(WsFType::ping)) {
                    cout << conn->ipaddress() + "<closed\n";
                    break;
                }
                cout << conn->ipaddress() + "<ping\n";
                se.pinged = true;
                se.timeoutcount = 0;
            }
            std::string data;
            if (auto e = r >> data) {
                conn->send_text(data.c_str());
            }
        }
    }
    roomlock.lock();
    leave_room(false, se.id, se.roomname, se.user);
    roomlock.unlock();
}

void handle_http(Recv r) {
    r.set_block(true);
    while (true) {
        std::shared_ptr<HttpServerConn> conn;
        if ((r >> conn) == commonlib2::ChanError::closed) {
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
        cout << "from " << conn->ipaddress() << ", with method ";
        auto found = conn->request().find(":method");
        cout << found->second << ", request path ";
        cout << conn->path() + conn->query() << ", respond ";
        auto path = normalize(conn->path());
        if (path == "ws") {
            auto wsconn = WebSocket::default_hijack_server_proc(conn);
            if (!wsconn) {
                cout << 400 << "\n";
                continue;
            }
            std::thread(handle_websocket, wsconn).detach();
            cout << 101 << "\n";
            continue;
        }
        {
            Reader r(FileReader(path.c_str()));
            if (r.ref().is_open()) {
                std::string data;
                r >> data;
                std::pair<std::string, std::string> ctype;
                ctype.first = "Content-Type";
                auto ext = path.extension();
                if (ext == "html" || ext == "htm") {
                    ctype.second = "text/html";
                }
                else if (ext == "js") {
                    ctype.second = "text/javascript";
                }
                else if (ext == "json") {
                    ctype.second = "application/json";
                }
                else if (ext == "css") {
                    ctype.second = "text/css";
                }
                else if (ext == "csv") {
                    ctype.second = "text/csv";
                }
                else if (ext == "txt") {
                    ctype.second = "text/plain";
                }
                Reader<ToUTF32<std::string>> tmp(data);
                if (tmp.ref().size()) {
                    if (!ctype.second.size()) {
                        ctype.second = "text/plain";
                    }
                    ctype.second += "; charset=UTF-8";
                }
                else {
                    ctype.second = "application/octet-stream";
                }
                conn->send(200, "OK", {ctype, {"Connection", "close"}}, data.c_str(), data.size());
                cout << 200 << "\n";
                continue;
            }
        }
        conn->send(404, "Not Found", {{"Content-Type", "text/html"}, {"Connection", "close"}}, "<h1>Page Not Found</h1>", 23);
        cout << 404 << "\n";
    }
}

int main(int argc, char** argv) {
    IOWrapper::Init();
    auto [w, r] = commonlib2::make_chan<std::shared_ptr<HttpServerConn>>(100);
    rooms["default"] = make_forkchan<std::string>();
    for (auto i = 0; i < std::thread::hardware_concurrency() - 1; i++) {
        std::thread(handle_http, r).detach();
    }
    Server sv;
    std::thread([&] {
        while (true) {
            std::string input;
            cin.getline(input);
            auto cmd = commonlib2::split_cmd(input);
            if (!cmd.size()) {
                continue;
            }
            for (auto& i : cmd) {
                if (is_string_symbol(i[0])) {
                    i.erase(0, 1);
                    i.pop_back();
                }
            }
            if (cmd[0] == "exit" || cmd[0] == "quit") {
                sv.set_suspend(true);
                break;
            }
            else if (cmd[0] == "cd") {
                if (cmd.size() < 2) {
                    auto dir = _wgetcwd(NULL, 0);
                    std::string n;
                    Reader(dir) >> n;
                    cout << n << "\n";
                    ::free(dir);
                    continue;
                }
                path_string path;
                Reader(cmd[1]) >> path;
                if (_wchdir(path.c_str()) != 0) {
                    cout << "cd: failed to change dir\n";
                }
                else {
                    cout << "cd: changed\n";
                }
            }
            else {
                cout << "no such command" << cmd[0] << "\n";
            }
        }
    }).detach();
    cout << "accept address:\n"
         << sv.ipaddress_list() << "\n";
    cout << "port:8080\n";
    while (true) {
        auto accept = Http1::serve(sv, 8080);
        if (!accept) {
            w.close();
            break;
        }
        w << std::move(accept);
    }
    Sleep(1000);
}
