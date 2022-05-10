#pragma once

#include "room_manager.h"
#include "user.h"

#include <ctime>

#include <memory>
#include <string>

#include <nlohmann/json.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

class WsUser : public IUser, public std::enable_shared_from_this<WsUser> {
    using Connection =
            websocketpp::server<websocketpp::config::asio>::connection_ptr;

public:
    ~WsUser() = default;

    WsUser(WsUser &) = delete;
    WsUser operator=(WsUser &) = delete;

public:
    std::string GetId() override;

public:
    WsUser(Connection con, std::string uuid,
           std::shared_ptr<RoomManager> manager);

    void OnRegist(const nlohmann::json &msg);
    void OnLogin(const nlohmann::json &msg);
    void OnCreateRoom(const nlohmann::json &msg);
    void OnJoinRoom(const nlohmann::json &msg);
    void OnLeaveRoom(const nlohmann::json &msg);
    void OnGetRoomList(const nlohmann::json &msg);
    void OnChangeOwner(const nlohmann::json &msg);
    void OnKickUser(const nlohmann::json &msg);
    void OnMute(const nlohmann::json &msg);
    void OnSendMsg(const nlohmann::json &msg);


private:
    void DoBroadcast(RoomManager::RoomId room_id, const std::string &&msg);

private:
    std::shared_ptr<RoomManager> manager_;
    Connection con_;
    RoomManager::RoomId room_id_;
    std::string uuid_;
    std::string device_name_;
};
