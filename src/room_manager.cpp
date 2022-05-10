#include "room_manager.h"

#include <iostream>

#include "log.h"

std::string RoomManager::ErrorCode2Str(ErrorCode ec) {
    std::string arr[] = {"succ", "room not exist", "already in other room",
                         "not in any room"};
    return arr[static_cast<uint16_t>(ec)];
}

RoomManager::RoomManager(RoomIdBuilder id_builder) : id_builder_(id_builder) {}

RoomManager::RoomId RoomManager::CreateRoom() {
    if (id_builder_ == nullptr) {
        for (uint32_t id = 100000;; ++id) {
            if (rooms_.find(id) == std::end(rooms_)) {
                return DoCreateRoom(id);
            }
        }
    } else {
        for (;;) {
            RoomId id = id_builder_();
            if (rooms_.find(id) == std::end(rooms_)) {
                return DoCreateRoom(id);
            }
        }
    }
}

RoomManager::RoomId RoomManager::CreateRoom(RoomId room_id) {
    if (rooms_.find(room_id) == std::end(rooms_)) {
        return DoCreateRoom(room_id);
    } else {
        throw "room_id already in use";
    }
}

RoomManager::ErrorCode RoomManager::JoinRoom(const std::shared_ptr<IUser> user,
                                             RoomId id) {
    if (rooms_.find(id) == std::end(rooms_)) {
        return ErrorCode::kRoomNotExist;
    }
    if (GetUserBelong(user->GetId()).first != 0) {
        return ErrorCode::kAlreadyInOtherRooms;
    }
    LOG_INFO("finish, user: {}, id: {}", user->GetId(), id);
    rooms_[id].push_back(user);
    return ErrorCode::kSucc;
}

RoomManager::ErrorCode RoomManager::LeaveRoom(std::string user_id,
                                              bool &is_last_one,
                                              RoomManager::RoomId &id) {
    // TODO : 优化
    auto res = GetUserBelong(user_id);
    id = res.first;
    auto iter = res.second;
    if (id == 0) { return ErrorCode::kNotInRoom; }

    rooms_[id].erase(iter);
    // 最后一个人退出时销毁房间
    is_last_one = false;
    if (rooms_[id].size() == 0) {
        rooms_.erase(id);
        is_last_one = true;
    }
    LOG_INFO("finish, user: {}, room: {}", user_id, id);
    return ErrorCode::kSucc;
}

std::vector<std::shared_ptr<IUser>> RoomManager::ListUser(RoomId id) {
    if (rooms_.find(id) == std::end(rooms_)) { return {}; }
    return rooms_[id];
}

std::shared_ptr<IUser> RoomManager::GetOwner(RoomId id) {
    if (rooms_.find(id) == std::end(rooms_)) { return nullptr; }
    auto owner = rooms_[id].size() ? rooms_[id][0] : nullptr;
    return owner;
}
std::vector<RoomManager::RoomInfo> RoomManager::GetRoomInfos() {
    std::vector<RoomManager::RoomInfo> ret;
    for (auto &pair : rooms_) {
        auto &room_id = pair.first;
        auto num_users = pair.second.size();
        auto owner = GetOwner(room_id);
        if (owner) { ret.push_back({room_id, num_users, owner->GetId()}); }
    }
    return ret;
}

RoomManager::RoomId RoomManager::DoCreateRoom(RoomId id) {
    rooms_[id] = {};
    std::cout << "RoomManager::DoCreateRoom finish: id = " << id << std::endl;
    return id;
}

std::pair<RoomManager::RoomId, std::vector<std::shared_ptr<IUser>>::iterator>
RoomManager::GetUserBelong(std::string user_id) {
    for (auto &pair : rooms_) {
        for (auto it = pair.second.begin(); it != pair.second.end(); ++it) {
            if ((*it)->GetId() == user_id) {
                return std::make_pair(pair.first, it);
            }
        }
    }
    return {};
}
