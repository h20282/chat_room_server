#pragma once

#include "user.h"

#include <functional>
#include <map>
#include <memory>
#include <vector>

class RoomManager {
public:
    using RoomId = uint32_t;
    using RoomIdBuilder = std::function<RoomId(void)>;
    enum class ErrorCode : uint16_t {
        kSucc = 0,
        kRoomNotExist = 1,
        kAlreadyInOtherRooms = 2,
        kNotInRoom = 3
    };

    struct RoomInfo {
        RoomId room_id;
        std::size_t num_users;
        std::string owner_id;
    };

public:
    RoomManager() = default;
    ~RoomManager() = default;

    RoomManager(RoomManager &) = delete;
    RoomManager operator=(RoomManager &) = delete;

public:
    static std::string ErrorCode2Str(ErrorCode ec);

public:
    RoomManager(RoomIdBuilder id_builder);

public:
    RoomId CreateRoom();
    RoomId CreateRoom(RoomId room_id);
    ErrorCode JoinRoom(const std::shared_ptr<IUser> user, RoomId id);
    ErrorCode LeaveRoom(std::string user_id, bool &is_last_one, RoomId &id);
    std::vector<std::shared_ptr<IUser>> ListUser(RoomId id);
    std::shared_ptr<IUser> GetOwner(RoomId id);
    std::vector<RoomInfo> GetRoomInfos();

private:
    RoomId DoCreateRoom(RoomId id);
    std::pair<RoomId, std::vector<std::shared_ptr<IUser>>::iterator>
    GetUserBelong(std::string user_id);

private:
    std::map<RoomId, std::vector<std::shared_ptr<IUser>>> rooms_;
    RoomIdBuilder id_builder_{nullptr};
};
