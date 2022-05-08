#include "ws_user.h"

#include <functional>
#include <iostream>
#include <map>

#include "log.h"

WsUser::WsUser(Connection con, std::string uuid,
               std::shared_ptr<RoomManager> manager)
        : con_(std::move(con)), uuid_(uuid), manager_(manager) {

    con_->set_message_handler(
            [this](websocketpp::connection_hdl hd,
                   websocketpp::server<websocketpp::config::asio>::message_ptr
                           message) {
                auto msg = message->get_payload();
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
        this->OnLeaveRoom({});
    });
}

std::string WsUser::GetId() {
    return uuid_;
}

void WsUser::OnRegist(const nlohmann::json &msg) {
    auto username = msg.at("userName").get<std::string>();
    auto password = msg.at("password").get<std::string>();
    // TODO: mysql

    auto result = true;

    nlohmann::json reply = {{"type", "reply.regist"}, {"result", result}};
    con_->send(reply.dump());
}
void WsUser::OnLogin(const nlohmann::json &msg) {
    auto username = msg.at("userName").get<std::string>();
    auto password = msg.at("password").get<std::string>();
    // TODO: mysql

    auto result = true;

    nlohmann::json reply = {{"type", "reply.login"}, {"result", result}};
    con_->send(reply.dump());
}
void WsUser::OnCreateRoom(const nlohmann::json &msg) {
    auto room_id = manager_->CreateRoom();
    manager_->JoinRoom(shared_from_this(), room_id);

    nlohmann::json reply = {{"type", "reply.createRoom"}, {"roomId", room_id}};
    con_->send(reply.dump());
}
void WsUser::OnJoinRoom(const nlohmann::json &msg) {
    auto room_id = msg.at("roomId").get<uint16_t>();
    // TODO password

    auto ec = manager_->JoinRoom(shared_from_this(), room_id);

    std::string reply_msg = RoomManager::ErrorCode2Str(ec);

    nlohmann::json reply = {{"type", "reply.joinRoom"},
                            {"result", ec == RoomManager::ErrorCode::kSucc},
                            {"msg", reply_msg}};
    con_->send(reply.dump());
}
void WsUser::OnLeaveRoom(const nlohmann::json &msg) {
    bool last_one;
    manager_->LeaveRoom(this->GetId(), last_one);
    if (last_one) {
        OnGetRoomList({});  // 主动向客户端同步房间列表
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
