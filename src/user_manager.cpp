#include "user_manager.h"

#include <algorithm>

void UserManager::AddUser(std::shared_ptr<IUser> user) {
    users_.push_back(user);
}
void UserManager::RemoveUser(std::shared_ptr<IUser> user) {
    auto iter = std::find(users_.begin(), users_.end(), user);
	auto p = *iter;
    if (iter != users_.end()) { users_.erase(iter); }
    offline_users_.push_back(p);
}
std::vector<std::shared_ptr<IUser>> UserManager::ListAllUsers() {
    return users_;
}
