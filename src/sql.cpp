#include <fmt/format.h>
#include <mysql.h>
#include <iostream>
#include <memory>
#include <vector>

using std::cin;
using std::cout;
using std::endl;
using std::string;
using std::vector;

namespace {
constexpr const char *kMysqlCharset = "gb2312";
}

class MysqlConnector {
public:
    MysqlConnector() = default;
    ~MysqlConnector() {
        mysql_close(conn_);
    }

public:
    bool Connect(const char *server, const char *user, const char *password,
                 const char *database) {
        conn_ = mysql_init(nullptr);
        mysql_set_character_set(conn_, kMysqlCharset);
        bool res = mysql_real_connect(conn_, server, user, password, database,
                                      0, NULL, 0);
        if (!res) { cout << "connect failed" << endl; }
        return res;
    }

    /* return nullptr if failed*/
    std::shared_ptr<vector<string>> SelectMysql(const char *sql, int n) {
        auto res = std::make_shared<vector<string>>();
        if (mysql_query(conn_, sql)) { return nullptr; }
        MYSQL_RES *data = mysql_store_result(conn_);
        if (!data) { return nullptr; }
        MYSQL_ROW record;
        while (record = mysql_fetch_row(data)) {
            for (int i = 0; i < n; ++i) { res->push_back(record[i]); }
        }
        return res;
    }

    bool UpdataMysql(const char *sql) {
        return mysql_query(conn_, sql) == 0;
    }

private:
    MYSQL *conn_{nullptr};
};

bool Login(string username, string password) {
    MysqlConnector connector;
    if (!connector.Connect("localhost", "root", "123", "user")) {
        return false;
    }
    // string sql = "select * from user where username = '" + username + "';";
    string sql =
            fmt::format("select * from user where username = '{}';", username);
    try {
        auto res = connector.SelectMysql(sql.c_str(), 3);
        for (auto item : *res) {
            cout << item << endl;
            if (item == password) { return true; }
        }
    } catch (std::logic_error e) {
        cout << e.what() << endl;
        return false;
    }
    return false;
}

bool Regist(string username, string password) {
    MysqlConnector connector;
    if (!connector.Connect("localhost", "root", "123", "user")) {
        return false;
    }
    string sql =
            fmt::format("select * from user where username = '{}';", username);
    try {
        auto res = connector.SelectMysql(sql.c_str(), 1);
        if (res->size() != 0) { return false; }
        string sql = fmt::format(
                "insert into user(username, passwd) values('{}', '{}');",
                username, password);
        return connector.UpdataMysql(sql.c_str());
    } catch (std::logic_error e) {
        cout << e.what() << endl;
        return false;
    }
}
