#include <string>
#include <vector>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>

struct User
{
    std::string name;
    std::string group;
    std::string role;
};

std::vector<User> make_users_list()
{
    mongocxx::instance inst{};
    mongocxx::client connection{ mongocxx::uri{"mongodb+srv://admin:qwertypassword@cluster0.ogjb22j.mongodb.net/?"} };
    mongocxx::v_noabi::collection collection = connection["Users_DataBase"]["Users"];

    std::vector<User> users;

    mongocxx::cursor cursor = collection.find({});

    for (const bsoncxx::v_noabi::document::view& document : cursor)
    {
        bsoncxx::document::element name = document["Name"];
        bsoncxx::document::element group = document["Group"];
        bsoncxx::document::element role = document["Role"];

        User user;

        user.name = std::string{ name.get_string().value };
        user.group = std::string{ group.get_string().value };
        user.role = std::string{ role.get_string().value };

        users.push_back(user);
    }

    return users;
}

void update_users_list(std::vector<User>* users, mongocxx::v_noabi::collection collection)
{
    mongocxx::cursor cursor = collection.find({});

    for (const bsoncxx::v_noabi::document::view& document : cursor)
    {
        bsoncxx::document::element name = document["Name"];
        bsoncxx::document::element group = document["Group"];
        bsoncxx::document::element role = document["Role"];

        User user;

        user.name = std::string{ name.get_string().value };
        user.group = std::string{ group.get_string().value };
        user.role = std::string{ role.get_string().value };

        bool is_present = false;
        for (const User& existing_user : *users)
        {
            if (existing_user.name == user.name && existing_user.group == user.group && existing_user.role == user.role)
            {
                is_present = true;
                break;
            }
        }

        if (!is_present)
        {
            users->push_back(user);
        }
    }
}