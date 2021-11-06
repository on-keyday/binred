#include <application/websocket.h>
#include <fileio.h>
#include <coutwrapper.h>
#include <channel.h>
#include <thread>
#include <filesystem>
#include <direct.h>
#include <optmap.h>
#include <argvlib.h>
using namespace socklib;
using namespace commonlib2;

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

std::pair<std::string, std::string> get_contenttype(std::string& data, std::filesystem::path& path) {
    std::pair<std::string, std::string> ctype;
    ctype.first = "Content-Type";
    auto ext = path.extension();
    if (ext == ".html" || ext == ".htm") {
        ctype.second = "text/html";
    }
    else if (ext == ".js") {
        ctype.second = "text/javascript";
    }
    else if (ext == ".json") {
        ctype.second = "application/json";
    }
    else if (ext == ".css") {
        ctype.second = "text/css";
    }
    else if (ext == ".csv") {
        ctype.second = "text/csv";
    }
    else if (ext == ".txt") {
        ctype.second = "text/plain";
    }
    Reader<ToUTF32<std::string>> tmp(data);
    if (tmp.ref().size()) {
        if (!ctype.second.size()) {
            ctype.second = "text/plain";
        }
        ctype.second += "; charset=UTF-8";
    }
    else if (!ctype.second.size()) {
        ctype.second = "application/octet-stream";
    }
    return ctype;
}

std::mutex roomlock;
std::map<std::string, ForkChan<std::string>> rooms;

struct WsSession {
    std::shared_ptr<WebSocketServerConn> conn;
    RecvChan<std::string> r;
    SendChan<std::string> w;
    WsSession(std::shared_ptr<WebSocketServerConn> conn, RecvChan<std::string> r, SendChan<std::string> w)
        : r(r), w(w), conn(conn) {}
    WsSession() {}
    size_t id = 0, timeoutcount = 0;
    bool pinged = false;
    std::string roomname = "default", user = "guest";
    bool loggedin = false;
};

std::string add_to_room(size_t& id, const std::string& name, const std::string& user, SendChan<std::string> rm) {
    std::string msg = "not joinable to room " + name;
    if (auto found = rooms.find(name); found != rooms.end()) {
        found->second.subscribe(id, rm);
        found->second << (const char*)u8"しすてむ>user " + user + "(" + std::to_string(id) + ") joined to " + name;
        msg = "joined to room " + name;
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
            found->second << (const char*)u8"しすてむ>" + user + "(" + std::to_string(id) + ") left from " + roomname;
        }
        found->second.remove(id);
        if (!found->second.size()) {
            if (found->first != "default") {
                rooms.erase(found->first);
            }
            else {
                found->second.reset_id();
            }
        }
    }
}

std::string parse_command(const std::string& str, WsSession& se, std::string& binarydata) {
    auto cmd = commonlib2::split(str, " ");
    if (!cmd.size()) {
        return "need any command";
    }
    if (cmd[0] == "login") {
        if (cmd.size() != 3) {
            return "need username and roomname";
        }
        if (cmd[1] == (const char*)u8"しすてむ") {
            return (const char*)u8"しすてむ is administrator name";
        }
        if (se.loggedin) {
            return "already logged in";
        }
        roomlock.lock();
        make_room(cmd[2]);
        add_to_room(se.id, cmd[2], cmd[1], se.w);
        roomlock.unlock();
        se.roomname = cmd[2];
        se.user = cmd[1];
        se.loggedin = true;
        return "login succeed";
    }
    else if (cmd[0] == "movroom") {
        if (cmd.size() != 2) {
            return "need room name";
        }
        if (!se.loggedin) {
            return "you are not logged in";
        }
        std::string msg = "not movable to room " + cmd[1];
        roomlock.lock();
        leave_room(false, se.id, se.roomname, se.user);
        make_room(cmd[1]);
        add_to_room(se.id, cmd[1], se.user, se.w);
        roomlock.unlock();
        se.roomname = cmd[1];
        return "moved to room " + cmd[1];
    }
    else if (cmd[0] == "close") {
        return "close";
    }
    else if (cmd[0] == "cast") {
        if (!se.loggedin) {
            return "you are not logged in";
        }
        commonlib2::Reader r(str);
        r.expect("cast");
        auto tosend = str.substr(r.readpos());
        tosend = se.user + "(" + std::to_string(se.id) + ")>" + tosend;
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
    else if (cmd[0] == "myid") {
        if (!se.loggedin) {
            return "you are not logged in";
        }
        return "id:" + std::to_string(se.id);
    }
    else if (cmd[0] == "myname") {
        if (!se.loggedin) {
            return "you are not logged in";
        }
        return "name:" + se.user;
    }
    else if (cmd[0] == "myroom") {
        if (!se.loggedin) {
            return "you are not logged in";
        }
        return "room:" + se.roomname;
    }
    else if (cmd[0] == "membercount") {
        if (!se.loggedin) {
            return "you are not logged in";
        }
        std::string msg = "you are not joined room";
        roomlock.lock();
        if (auto found = rooms.find(se.roomname); found != rooms.end()) {
            msg = "member:" + std::to_string(found->second.size());
        }
        roomlock.unlock();
        return msg;
    }
    else if (cmd[0] == "roomcount") {
        if (!se.loggedin) {
            return "you are not logged in";
        }
        size_t count = 0;
        roomlock.lock();
        count = rooms.size();
        roomlock.unlock();
        return "roomcount:" + std::to_string(count);
    }
    else if (cmd[0] == "chname") {
        if (cmd.size() != 2) {
            return "need name";
        }
        if (!se.loggedin) {
            return "you are not logged in";
        }
        if (cmd[1] == (const char*)u8"しすてむ") {
            return (const char*)u8"しすてむ is administrator name";
        }
        if (se.user == cmd[1]) {
            return "";
        }
        roomlock.lock();
        if (auto found = rooms.find(se.roomname); found != rooms.end()) {
            found->second << (const char*)u8"しすてむ>change name from " + se.user + "(" + std::to_string(se.id) + ") to " + cmd[1];
        }
        roomlock.unlock();
        se.user = cmd[1];
        return "";
    }
    else if (cmd[0] == "file") {
        if (cmd.size() != 2) {
            return "need file name";
        }
        auto path = normalize(cmd[1]);
        path_string pathstr;
        Reader(path.c_str()) >> pathstr;
        Reader tmp(FileReader(pathstr.c_str()));
        if (!tmp.ref().is_open()) {
            return "no such file";
        }
        tmp >> binarydata;
        auto ctype = get_contenttype(binarydata, path);
        return ctype.first + ":" + ctype.second;
    }
    return "no such command:" + cmd[1];
}

void websocket_thread(SendChan<WsSession> w, RecvChan<WsSession> r) {
    r.set_block(true);
    cout << std::this_thread::get_id() << ":websocket thread start\n";
    while (true) {
        WsSession se;
        try {
            if ((r >> se) == ChanError::closed) {
                cout << std::this_thread::get_id() << ":websocket thread closed\n";
                return;
            }
            auto conn = se.conn;
            bool leave = true;
            while (true) {
                if (conn->recvable() || Selecter::waitone(conn->borrow(), 0, 1)) {
                    WsFrame frame;
                    if (!conn->recv(frame)) {
                        cout << conn->ipaddress() << ">closed\n";
                        return;
                    }
                    if (frame.frame_type("ping")) {
                        cout << conn->ipaddress() + ">ping\n";
                        conn->control(WsFType::pong, frame.get_data().c_str(), frame.get_data().size());
                        cout << conn->ipaddress() + "<pong\n";
                    }
                    else if (frame.frame_type("pong")) {
                        se.pinged = false;
                        cout << conn->ipaddress() + ">pong\n";
                    }
                    else if (frame.frame_type("text")) {
                        cout << conn->ipaddress() << ">data\n";
                        cout << frame.get_data() << "\n";
                        std::string data;
                        auto resp = parse_command(frame.get_data(), se, data);
                        if (resp.size()) {
                            cout << conn->ipaddress() << "<data\n";
                            if (resp == "close") {
                                cout << "logout\n";
                                conn->send_text("logout");
                                cout << conn->ipaddress() << "<close\n";
                                conn->control(WsFType::closing, "\x03\xe8", 2);
                                break;
                            }
                            else {
                                cout << resp << "\n";
                                conn->send_text(resp.c_str());
                            }
                        }
                        if (data.size()) {
                            conn->send(data.c_str(), data.size());
                            cout << conn->ipaddress() << "<binary data\n";
                        }
                    }
                    se.timeoutcount = 0;
                }
                else {
                    se.timeoutcount++;
                    if (se.pinged && se.timeoutcount > 300) {
                        cout << conn->ipaddress() + "<closed\n";
                        conn->control(WsFType::closing, "\x03\xe8", 2);
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
                    if (auto e = se.r >> data) {
                        cout << conn->ipaddress() << "<cast data\n";
                        cout << data << "\n";
                        conn->send_text(data.c_str());
                    }
                }
                if (conn->recvable()) {
                    continue;
                }
                w << std::move(se);
                leave = false;
                break;
            }
            if (leave) {
                roomlock.lock();
                leave_room(false, se.id, se.roomname, se.user);
                roomlock.unlock();
            }
        } catch (...) {
            cout << "exception thrown\n";
            if (se.conn) {
                cout << "conn to " << se.conn->ipaddress() << " lost\n";
                roomlock.lock();
                leave_room(false, se.id, se.roomname, se.user);
                roomlock.unlock();
            }
        }
    }
}

void handle_websocket(std::shared_ptr<WebSocketServerConn> conn, SendChan<WsSession> ws) {
    cout << conn->ipaddress() << ">connect\n";
    auto [w, r] = commonlib2::make_chan<std::string>(100);
    ws << WsSession(conn, r, w);
}

struct HttpSession {
    std::shared_ptr<HttpServerConn> conn;
    time_t actime = 0;
};

void handle_http(RecvChan<HttpSession> r, SendChan<HttpSession> s, SendChan<WsSession> ws, RecvChan<WsSession> rs) {
    r.set_block(true);
    cout << std::this_thread::get_id() << ":http thread start\n";
    std::thread(websocket_thread, ws, rs).detach();
    while (true) {
        HttpSession session;
        bool sent = false;
        try {
            if ((r >> session) == commonlib2::ChanError::closed) {
                ws.close();
                cout << std::this_thread::get_id() << ":http thread closed\n";
                break;
            }
            if (!session.conn) {
                continue;
            }
            if (session.actime != 0) {
                if (std::time(nullptr) - session.actime >= 5) {
                    cout << session.conn->ipaddress() << " end keep-alive\n";
                    continue;
                }
            }
            auto conn = session.conn;
            if (session.actime != 0) {
                if (!Selecter::waitone(conn->borrow(), 0, 1)) {
                    s << std::move(session);
                    continue;
                }
            }
            TimeoutContext timeout(10);
            SleeperContext ctx(&timeout);
            if (!conn->recv(&timeout)) {
                continue;
            }
            cout << "from " << conn->ipaddress() << ", with method ";
            auto found = conn->request().find(":method");
            cout << found->second << ", request path ";
            cout << conn->path() + conn->query() << ", respond ";
            auto path = normalize(conn->path());
            std::pair<std::string, std::string> connstate;
            connstate.first = "Connection";
            connstate.second = "close";
            bool keepalive = false;
            if (auto found = conn->request().find("connection"); found != conn->request().end()) {
                if (found->second == "keep-alive" || found->second == "Keep-Alive") {
                    connstate.second = "keep-alive; maxage=5";
                    keepalive = true;
                }
            }
            if (path == "ws") {
                auto wsconn = WebSocket::default_hijack_server_proc(conn);
                if (!wsconn) {
                    cout << 400 << "\n";
                    continue;
                }
                cout << 101 << "\n";
                handle_websocket(wsconn, ws);
                continue;
            }
            {
                path_string pathstr;
                Reader(path.c_str()) >> pathstr;
                Reader r(FileReader(pathstr.c_str()));
                if (r.ref().is_open()) {
                    std::string data;
                    r >> data;
                    auto ctype = get_contenttype(data, path);
                    sent = true;
                    conn->send(200, "OK", {ctype, connstate}, data.c_str(), data.size());
                    cout << 200;
                    if (keepalive) {
                        cout << " keep-alive";
                        session.actime = std::time(nullptr);
                        s << std::move(session);
                    }
                    cout << "\n";
                    continue;
                }
            }
            sent = true;
            conn->send(404, "Not Found", {{"Content-Type", "text/html"}, connstate}, "<h1>Page Not Found</h1>", 23);
            cout << 404;
            if (keepalive) {
                cout << " keep-alive";
                session.actime = std::time(nullptr);
                s << std::move(session);
            }
            cout << "\n";
        } catch (...) {
            cout << "exception thrown\n";
            if (!sent && session.conn) {
                session.conn->send(500, reason_phrase(500));
            }
        }
    }
}

int main(int argc, char** argv) {
    IOWrapper::Init();
    OptMap option;
    option.set_option({
        {"port", {'p'}, "set portnumber", 1, true},
        {"rootdir", {'r', 'c'}, "set root directory", 1, true},
        {"logfile", {'f'}, "set logfile", 1, true},
        {"help", {'h'}, "show help"},
    });
    ArgChange _(argc, argv);
    OptMap<>::OptResMap result;
    option.set_usage(argv[0] + std::string(" <options>"));
    auto e = option.parse_opt(argc, argv, result);
    if (!e) {
        cout << "webserver: " << error_message(e) << "\n";
        return -1;
    }
    auto change_dir = [](auto& dir) {
        path_string path;
        Reader(dir) >> path;
        if (_wchdir(path.c_str()) != 0) {
            cout << "cd: failed to change dir\n";
        }
        else {
            cout << "cd: changed\n";
        }
    };
    if (result.has_("help")) {
        cout << option.help();
        return 0;
    }
    std::uint16_t port = 8080;
    if (auto v = result.has_("port")) {
        Reader((*v->arg())[0]) >> port;
        if (port == 0) {
            port = 8080;
        }
    }
    if (auto v = result.has_("rootdir")) {
        change_dir((*v->arg())[0]);
    }
    if (auto v = result.has_("logfile")) {
        if (!cout.get().open((*v->arg())[0])) {
            cout << "webserver: can't open file" << (*v->arg())[0]
                 << "\n";
            return -1;
        }
        cout.get().set_multiout(true);
    }
    auto [w, r] = commonlib2::make_chan<HttpSession>(500000);
    auto [ws, wr] = commonlib2::make_chan<WsSession>(500000);
    rooms.emplace("default", make_forkchan<std::string>());
    auto hrd = std::thread::hardware_concurrency();
    for (auto i = 0; i < hrd / 2; i++) {
        std::thread(handle_http, r, w, ws, wr).detach();
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
            remove_strsymbol(cmd);
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
                change_dir(cmd[1]);
            }
            else {
                cout << "no such command " << cmd[0] << "\n";
            }
        }
    }).detach();
    Sleep(500);
    cout << "accept address:\n"
         << sv.ipaddress_list() << "\n";
    cout << "port:8080\n";
    while (true) {
        auto accept = Http1::serve(sv, 8080);
        if (!accept) {
            w.close();
            break;
        }
        w << HttpSession{accept, 0};
    }
    Sleep(2000);
}
