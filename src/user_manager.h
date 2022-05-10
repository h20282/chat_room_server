#pragma once

#include <memory>
#include <vector>

#include "user.h"

class UserManager {
public:
    void AddUser(std::shared_ptr<IUser>);
    void RemoveUser(std::shared_ptr<IUser>);
    std::vector<std::shared_ptr<IUser>> ListAllUsers();

private:
    std::vector<std::shared_ptr<IUser>> users_;
    std::vector<std::shared_ptr<IUser>> offline_users_;
};
