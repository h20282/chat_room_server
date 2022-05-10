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
/*
void WsUser::OnLogin(const nlohmann::json &msg) {
    nlohmann::json reply;

    if (uuid_ != std::string()) {
        nlohmann::json reply = {{"type", ResType::kLogin},
                                {"timeStamp", time(nullptr)},
                                {"code", -1},
                                {"msg", "重复登录"},
                                {"uuid", uuid_}};
        con_->send(reply.dump());
        return;
    }

    device_name_ = msg.at("deviceName").get<std::string>();
    if (msg.at("platform").get<std::string>() == "broad") {
        uuid_ = msg.at("SN").get<std::string>();
        room_id_ = manager_->CreateRoom(100);
        manager_->JoinRoom(shared_from_this(), room_id_);
        reply = {{"type", ResType::kLogin},
                 {"timeStamp", time(nullptr)},
                 {"code", 200},
                 {"msg", "登陆成功"},
                 {"roomId", room_id_},
                 {"uuid", uuid_}};
    } else {
        uuid_ = std::to_string(time(nullptr) % 1000007);
        reply = {{"type", ResType::kLogin},
                 {"timeStamp", time(nullptr)},
                 {"code", 200},
                 {"msg", "登陆成功"},
                 {"uuid", uuid_}};
    }
    con_->send(reply.dump());
}

void WsUser::OnJoinRoom(const nlohmann::json &msg) {
    if (uuid_ == "") {
        nlohmann::json reply = {{"type", ReqType::kJoinRoom},
                                {"msg", "未登录a"},
                                {"timeStamp", time(nullptr)},
                                {"code", -1}};
        auto msg = reply.dump();
        con_->send(msg);
        return;
    }
    RoomManager::RoomId room_id;
    try {
        room_id = std::stoul(msg.at("roomId").get<std::string>());
    } catch (nlohmann::json::exception) {
        room_id = (msg.at("roomId").get<RoomManager::RoomId>());
    }
    auto ec = manager_->JoinRoom(shared_from_this(), room_id);

    nlohmann::json replay{
            {"type", ResType::kJoinRoom},
            {"timeStamp", time(nullptr)},
            {"code", ec == RoomManager::ErrorCode::kSucc ? 200 : -1},
            {"msg", RoomManager::ErrorCode2Str(ec)}};
    con_->send(replay.dump());

    // broadcast
    if (ec == RoomManager::ErrorCode::kSucc) {
        DoBroadcast(room_id, std::move(nlohmann::json{
                                     {"type", ResType::kJoinRoomBroadcast},
                                     {"timeStamp", time(nullptr)},
                                     {"uuid", uuid_},
                                     {"deviceName", device_name_}}
                                               .dump()));
        room_id_ = room_id;
    }
}
void WsUser::OnLeaveRoom(const nlohmann::json &msg) {
    // auto room_id = msg.at("roomId").get<std::string>();
    RoomManager::RoomId id;
    auto ec = manager_->LeaveRoom(uuid_, id);
    con_->send(nlohmann::json{
            {"type", ResType::kLeaveRoom},
            {"timeStamp", time(nullptr)},
            {"code", ec == RoomManager::ErrorCode::kSucc ? 200 : -1},
            {"msg", RoomManager::ErrorCode2Str(ec)}}
                       .dump());
    if (ec == RoomManager::ErrorCode::kSucc) {
        DoBroadcast(id, nlohmann::json{{"type", ResType::kLeaveRoomBroadcast},
                                       {"timeStamp", time(nullptr)},
                                       {"uuid", uuid_},
                                       {"deviceName", device_name_}}
                                .dump());
    }
}
void WsUser::OnForward(const nlohmann::json &msg) {
    if (uuid_ != msg.at("fromUUID").get<std::string>()) {
        // TODO: check
    }
    std::string to;

    if (msg.at("targetType").get<uint8_t>() == 1) {
        to = msg.at("to").get<std::string>();
    } else {
        RoomManager::RoomId room_id =
                std::stoul(msg.at("to").get<std::string>());
        to = manager_->GetOwner(room_id)->GetId();
    }
    auto msg_copy = msg;
    msg_copy["fromUuid"] = msg["fromUUID"];
    for (auto user : manager_->ListUser(room_id_)) {
        if (user->GetId() == to) {
            dynamic_cast<WsUser *>(user.get())->con_->send(msg_copy.dump());
            std::cout << "finded dosend" << std::endl;
            return;
        }
    }
    std::cout << "user not find:" << to << " from " << room_id_ << std::endl;
}

void WsUser::DoBroadcast(RoomManager::RoomId room_id, const std::string &&msg) {
    for (auto user : manager_->ListUser(room_id)) {
        dynamic_cast<WsUser *>(user.get())->con_->send(msg);
    }
}
*/
