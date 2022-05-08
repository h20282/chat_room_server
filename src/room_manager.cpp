#include "room_manager.h"

#include <iostream>

std::string RoomManager::ErrorCode2Str(ErrorCode ec) {
    std::string arr[] = {"succ", "room not exist", "already in other room",
                         "not in any room"};
    return arr[static_cast<uint16_t>(ec)];
}

RoomManager::RoomManager(RoomIdBuilder id_builder) : id_builder_(id_builder) {}

RoomManager::RoomId RoomManager::CreateRoom(RoomId room_id) {
    if (rooms_.find(room_id) == std::end(rooms_)) {
        return DoCreateRoom(room_id);
    }

    if (id_builder_ == nullptr) {
        for (uint32_t id = 0;; ++id) {
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

RoomManager::ErrorCode RoomManager::JoinRoom(const std::shared_ptr<IUser> user,
                                             RoomId id) {
    if (rooms_.find(id) == std::end(rooms_)) {
        return ErrorCode::kRoomNotExist;
    }
    if (GetUserBelong(user->GetId()).first != 0) {
        return ErrorCode::kAlreadyInOtherRooms;
    }
    std::cout << "RoomManager::JoinRoom finish, user = " << user->GetId()
              << ", id = " << id << std::endl;
    rooms_[id].push_back(user);
    return ErrorCode::kSucc;
}

std::pair<RoomManager::ErrorCode, RoomManager::RoomId> RoomManager::LeaveRoom(
        std::string user_id) {
    // TODO : 优化
    auto res = GetUserBelong(user_id);
    auto id = res.first;
    auto iter = res.second;
    if (id == 0) { return std::make_pair(ErrorCode::kNotInRoom, id); }

    rooms_[id].erase(iter);
    std::cout << "RoomManager::LeaveRoom finish, user = " << user_id
              << ", room = " << id << std::endl;
    return std::make_pair(ErrorCode::kSucc, id);
}

std::vector<std::shared_ptr<IUser>> RoomManager::ListUser(RoomId id) {
    if (rooms_.find(id) == std::end(rooms_)) { return {}; }
    return rooms_[id];
}
std::shared_ptr<IUser> RoomManager::GetOwner(RoomId id) {
    if (rooms_.find(id) == std::end(rooms_)) { return nullptr; }
    auto owner = rooms_[id].size() ? rooms_[id][0] : nullptr;
    std::cout << "RoomManager::GetOwner finish, owner = " << owner->GetId()
              << std::endl;
    return owner;
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
