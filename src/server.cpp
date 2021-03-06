
#include "room_manager.h"

#include "ws_user.h"

#include <ctime>
#include <iostream>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "log.h"
#include "user_manager.h"

using WsServer = websocketpp::server<websocketpp::config::asio>;

WsServer room_server;
std::vector<std::shared_ptr<WsUser>> users;

UserManager g_user_manager;

auto manager = std::make_shared<RoomManager>();
// void Send(websocketpp::connection_hdl hd) {
//     room_server.get_con_from_hdl(hd);
//     auto arr = nlohmann::json(msgs).dump();
//     room_server.send(hd, arr, websocketpp::frame::opcode::value::text);
// }

void OnMessage(websocketpp::connection_hdl hd, WsServer::message_ptr msg_p) {
    // room_server.get_con_from_hdl(hd)->set_message_handler();
    auto msg = msg_p->get_payload();
    // msgs.push_back(std::make_pair(std::time(0), msg));
    // Send(hd);
}

void OnOpen(websocketpp::connection_hdl hd) {
    auto con = room_server.get_con_from_hdl(hd);
    auto new_user = std::make_shared<WsUser>(con, "", manager);
    g_user_manager.AddUser(new_user);
    // WsUser(manager);
}

int main() {

    spdlog::set_pattern("[%Y-%m-%d %H:%M:%s %S.%e][%t][%l][%!:%#]:%v");

    room_server.set_message_handler(&OnMessage);
    room_server.set_open_handler(&OnOpen);
    room_server.set_access_channels(websocketpp::log::alevel::all);
    room_server.set_error_channels(websocketpp::log::elevel::all);

    room_server.clear_access_channels(websocketpp::log::alevel::all);

    room_server.init_asio();
    room_server.listen(9001);
    room_server.start_accept();

    room_server.run();
}
