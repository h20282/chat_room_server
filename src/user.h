#pragma once

#include <string>

class IUser {
public:
    virtual std::string GetId() = 0;
};
