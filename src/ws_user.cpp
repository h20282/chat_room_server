#include "ws_user.h"

#include <functional>
#include <iostream>
#include <map>

#include "log.h"
#include "sql.h"
#include "user_manager.h"

extern UserManager g_user_manager;

WsUser::WsUser(Connection con, std::string uuid,
               std::shared_ptr<RoomManager> manager)
        : con_(std::move(con)), uuid_(uuid), manager_(manager) {

    con_->set_message_handler(
            [this](websocketpp::connection_hdl hd,
                   websocketpp::server<websocketpp::config::asio>::message_ptr
                           message) {
                auto msg = message->get_payload();
                LOG_INFO("recv msg : `{}`", msg);
                try {

                    auto json = nlohmann::json::parse(msg);

                    auto type = json.at("type").get<std::string>();
                    const static std::map<std::string,
                                          void (WsUser::*)(
                                                  const nlohmann::json &)>
                            handlers = {
                                    {"request.regist", &WsUser::OnRegist},
                                    {"request.login", &WsUser::OnLogin},
                                    {"request.createRoom",
                                     &WsUser::OnCreateRoom},
                                    {"request.joinRoom", &WsUser::OnJoinRoom},
                                    {"request.leaveRoom", &WsUser::OnLeaveRoom},
                                    {"request.getRoomList",
                                     &WsUser::OnGetRoomList},
                                    {"request.changeOwner",
                                     &WsUser::OnChangeOwner},
                                    {"request.kickUser", &WsUser::OnKickUser},
                                    {"request.mute", &WsUser::OnMute},
                                    {"request.sendMsg", &WsUser::OnSendMsg},
                            };
                    auto handler = handlers.at(type);
                    (this->*handler)(json);
                } catch (nlohmann::json::exception e) {
                    LOG_ERROR("{} when hand msg: `{}`", e.what(), msg);
                } catch (std::exception e) {
                    LOG_ERROR("{} when hand msg: `{}`", e.what(), msg);
                }
            });
    con_->set_close_handler([this](websocketpp::connection_hdl) {
        LOG_ERROR("close");
        g_user_manager.RemoveUser(shared_from_this());
        this->OnLeaveRoom({});
    });
}

std::string WsUser::GetId() {
    return uuid_;
}

void WsUser::OnRegist(const nlohmann::json &msg) {
    auto username = msg.at("userName").get<std::string>();
    auto password = msg.at("password").get<std::string>();

    bool result = Regist(username, password);
    std::string reply_msg = result ? "succ" : "username already exists";

    nlohmann::json reply = {
            {"type", "reply.regist"}, {"result", result}, {"msg", reply_msg}};
    con_->send(reply.dump());
}
void WsUser::OnLogin(const nlohmann::json &msg) {
    auto username = msg.at("userName").get<std::string>();
    auto password = msg.at("password").get<std::string>();

    std::string reply_msg = "succ";
    bool already_online = false;
    for (auto user : g_user_manager.ListAllUsers()) {
        if (user->GetId() == username) {
            already_online = true;
            reply_msg = "re login";
            break;
        }
    }

    bool username_and_password_right = false;
    if (!already_online) {
        username_and_password_right = Login(username, password);
        if (!username_and_password_right) {
            reply_msg = "username or password error";
        }
    }

    auto result = !already_online && username_and_password_right;
    this->uuid_ = username;

    nlohmann::json reply = {
            {"type", "reply.login"}, {"result", result}, {"msg", reply_msg}};
    con_->send(reply.dump());
    if (!result) { con_->close({}, {}); }
}
void WsUser::OnCreateRoom(const nlohmann::json &msg) {
    auto room_id = manager_->CreateRoom();

    nlohmann::json reply = {{"type", "reply.createRoom"}, {"roomId", room_id}};
    con_->send(reply.dump());

    nlohmann::json broadcast = {
            {"type", "boardcast.createRoom"},
            {"roomId", room_id},
            {"ownerId", GetId()},
    };
    auto boardcast_msg = broadcast.dump();
    auto users = g_user_manager.ListAllUsers();
    for (auto user : users) {
        LOG_INFO("boardcast.createRoom to `{}`", user->GetId());
        dynamic_cast<WsUser *>(user.get())->con_->send(boardcast_msg);
    }
}
void WsUser::OnJoinRoom(const nlohmann::json &msg) {
    auto room_id = msg.at("roomId").get<uint32_t>();
    // TODO password

    auto ec = manager_->JoinRoom(shared_from_this(), room_id);

    std::string error_msg = RoomManager::ErrorCode2Str(ec);

    auto result = ec == RoomManager::ErrorCode::kSucc;
    nlohmann::json reply = {
            {"type", "reply.joinRoom"}, {"result", result}, {"msg", error_msg}};
    con_->send(reply.dump());
}
void WsUser::OnLeaveRoom(const nlohmann::json &msg) {
    bool last_one;
    RoomManager::RoomId room_id;
    auto ec = manager_->LeaveRoom(this->GetId(), last_one, room_id);
    LOG_INFO("user `{}` leaveRoom, is_last_one: {}, room_id: {}", this->GetId(),
             last_one, room_id);

    bool result = ec == RoomManager::ErrorCode::kSucc;
    std::string error_msg = RoomManager::ErrorCode2Str(ec);
    nlohmann::json reply = {{"type", "reply.leaveRoom"},
                            {"result", result},
                            {"msg", error_msg}};
    con_->send(reply.dump());

    nlohmann::json boardcast = {
            {"type", "boardcast.leaveRoom"},
            {"userId", this->GetId()},
    };

    auto boardcast_msg = boardcast.dump();
    auto users = manager_->ListUser(room_id);
    for (auto user : users) {
        dynamic_cast<WsUser *>(user.get())->con_->send(boardcast_msg);
    }
    if (last_one) {
        for (auto user : users) {
            dynamic_cast<WsUser *>(user.get())
                    ->OnGetRoomList({});  // 主动向客户端同步房间列表
        }
    }
}
void WsUser::OnGetRoomList(const nlohmann::json &msg) {
    nlohmann::json rooms;
    for (auto room_info : manager_->GetRoomInfos()) {
        rooms.push_back(nlohmann::json{{"roomId", room_info.room_id},
                                       {"userNum", room_info.num_users},
                                       {"ownerId", room_info.owner_id}});
    }
    nlohmann::json reply = {{"type", "reply.getRoomList"}, {"rooms", rooms}};
    con_->send(reply.dump());
}
void WsUser::OnChangeOwner(const nlohmann::json &msg) {}
void WsUser::OnKickUser(const nlohmann::json &msg) {}
void WsUser::OnMute(const nlohmann::json &msg) {}

void WsUser::OnSendMsg(const nlohmann::json &msg) {
    auto user_msg = msg.at("text").get<std::string>();

    auto room_id = manager_->GetRoomIdFromUser(uuid_);
    auto users = manager_->ListUser(room_id);

    for (auto user : users) {
        nlohmann::json boardcast_msg{
                {"type", "boardcast.sendMsg"},
                {"user", uuid_},
                {"text", user_msg},
        };
        LOG_INFO("sendMsg -> {}", user->GetId());
        dynamic_cast<WsUser *>(user.get())->con_->send(boardcast_msg.dump());
    }
}
