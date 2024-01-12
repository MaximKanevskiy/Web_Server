#include <iostream>
#include <direct.h>
#include <httplib.h>
#include <cookie.h>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>
#include "users_list.hpp"

using namespace std;
using namespace httplib;

using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::stream::close_document;

std::string generateCookie(int cookie_size)
{
    std::string letters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::string new_cookie = "";

    std::mt19937 generator(std::chrono::system_clock::now().time_since_epoch().count());

    for (int i = 0; i < cookie_size; i++)
    {
        std::uniform_int_distribution<int> distribution(0, letters.length() - 1);
        int randomIndex = distribution(generator);
        new_cookie += letters[randomIndex];
    }

    return new_cookie;
}

using session_data = std::map<std::string, std::string>;
std::map<std::string, session_data> sessions;

mongocxx::client connection{ mongocxx::uri{"mongodb+srv://admin:qwertypassword@cluster0.ogjb22j.mongodb.net/?"} };
mongocxx::v_noabi::collection collection = connection["Users_DataBase"]["Users"];

std::vector<User> users = make_users_list();

httplib::Server::Handler loginСheck(httplib::Server::Handler next);
void loginHandler(const Request& req, Response& res);
void homeHandler(const Request& req, Response& res);
void logoutHandler(const Request& req, Response& res);
void updateRoleHandler(const Request& req, Response& res);
void deleteUserHandler(const Request& req, Response& res);
void uploadFileHandler(const Request& req, Response& res);
void sendFileHandler(const Request& req, Response& res);

int main() 
{
    Server server;
    server.Get("/", loginСheck(homeHandler));
    server.Get("/login", loginСheck(loginHandler));
    server.Post("/login", loginСheck(loginHandler));
    server.Get("/logout", loginСheck(logoutHandler));
    server.Post("/update-role", loginСheck(updateRoleHandler));
    server.Post("/delete-user", loginСheck(deleteUserHandler));
    server.Post("/upload-file", loginСheck(uploadFileHandler));
    server.Post("/send-file", loginСheck(sendFileHandler));

    server.listen("0.0.0.0", 8080);
}

static bool is_session_exist(const Request& req) 
{
    auto cookie = Cookie::get_cookie(req, "user_cookie");

    if (cookie.name == "") return false;

    if (sessions.count(cookie.value) == 0) return false;

    return true;
}

httplib::Server::Handler loginСheck(httplib::Server::Handler next) 
{
    return [next](const Request& req, Response& res) 
    {
        if (!is_session_exist(req) && req.path != "/login") 
        {
            res.set_redirect("/login");
            return;
        }

        if (is_session_exist(req) && req.path == "/login") 
        {
            res.set_redirect("/");
            return;
        }

        next(req, res);
    };
}

void loginHandler(const Request& req, Response& res) 
{
    if (req.method == "GET") 
    {
        std::string page = u8R"#(
            <!DOCTYPE html>
            <html>
            <head>
		        <meta charset="UTF-8">
		        <meta name="viewport" content="width=device-width, initial-scale=1.0">
		        <title>Login page</title>
                <style>
                    body {
                        background-color: #f0f0f0;
                        font-family: Arial, sans-serif;
                    }
                    .mainContainer {
                        position: absolute;
                        top: 50%;
                        left: 50%;
                        transform: translate(-50%, -50%);
                        padding: 20px;
                        background-color: #fff;
                        box-shadow: 0px 0px 10px rgba(0,0,0,0.1);
                        border-radius: 10px;
                    }
                    label {
                        display: inline-block;
                        margin-bottom: 5px;
                    }
                    input[type="text"], input[type="password"] {
                        width: 94%;
                        padding: 10px;
                        margin-bottom: 10px;
                        border-radius: 5px;
                        border: 1px solid #ccc;
                    }
                    .btn-new {
                        width: 60px;
                        height: 35px;
                        border-radius: 10px;
                        color: white;
                        transition: .2s linear;
                        background: #0B63F6;
                    }
                    .btn-new:hover {
                        box-shadow: 0 0 0 2px white, 0 0 0 4px #3C82F8;
                    }
                </style>
            </head>
            <body>
		        <form method="post" action="/login">
				    <div class="mainContainer">
						<label for="username"></label>
						<input type="text" autocomplete="off" placeholder="Введите имя" name="username" required>
						<label for="password"></label>
                        <input type="password" autocomplete="off" placeholder="Введите пароль" name="password" required>
						<button class="btn-new" type="submit">Вход</button>
				    </div>
		        </form>
            </body>
            </html>)#";

        res.set_content(page, "text/html");
    }

    if (req.method == "POST") 
    {
        const std::string correct_password = "15123415";

        auto username = req.has_param("username") ? req.get_param_value("username") : "";
        auto password = req.has_param("password") ? req.get_param_value("password") : "";

        if (username != "" && password == correct_password)
        {
            Cookie cookie;
            cookie.name = "user_cookie";
            cookie.value = generateCookie(16);
            cookie.path = "/";
            cookie.maxAge = 3600;
            cookie.httpOnly = true;
            cookie.sameSite = Cookie::SameSiteLaxMode;

            sessions[cookie.value]["username"] = username;

            Cookie::set_cookie(res, cookie);

            std::cout << username << " has been connected with cookie: " << cookie.value << std::endl;
        }

        res.set_redirect("/");
    }
}

void homeHandler(const Request& req, Response& res) 
{
    auto cookie = Cookie::get_cookie(req, "user_cookie");
    auto& session = sessions[cookie.value];

    std::string html_response;

    if (req.method == "GET")
    {
        html_response += u8"<html>";
        html_response += u8"<header>";
        html_response += u8"<meta charset = 'utf-8'>";
        html_response += u8"<title>Administration page</title>";
        html_response += u8"<style>";
        html_response += u8"* {";
        html_response += u8"  font-family: Verdana, sans-serif;";
        html_response += u8"}";
        html_response += u8"table {";
        html_response += u8"  border: 1px solid #ddd;";
        html_response += u8"  border-collapse: collapse;";
        html_response += u8"  width: 50%;";
        html_response += u8"}";
        html_response += u8"th, td {";
        html_response += u8"  text-align: left;";
        html_response += u8"  border-bottom: 1px solid #ddd;";
        html_response += u8"}";
        html_response += u8"tr:hover {";
        html_response += u8"  background-color: #f5f5f5;";
        html_response += u8"}";
        html_response += u8".logout-button {";
        html_response += u8"  background-color: #4B535C;";
        html_response += u8"  color: white;";
        html_response += u8"  padding: 10px 20px;";
        html_response += u8"  border: none;";
        html_response += u8"  border-radius: 10px;";
        html_response += u8"  text-align: center;";
        html_response += u8"  text-decoration: none;";
        html_response += u8"  display: inline-block;";
        html_response += u8"  font-size: 16px;";
        html_response += u8"  margin: 4px 2px;";
        html_response += u8"  cursor: pointer;";
        html_response += u8"}";
        html_response += u8".delete-button {";
        html_response += u8"  background-color: #B30000;";
        html_response += u8"  color: white;";
        html_response += u8"  border: none;";
        html_response += u8"  border-radius: 10px";
        html_response += u8"  text-align: center;";
        html_response += u8"  text-decoration: none;";
        html_response += u8"  display: inline-block;";
        html_response += u8"  font-size: 12px;";
        html_response += u8"  cursor: pointer;";
        html_response += u8"}";
        html_response += u8".inp {";
        html_response += u8"  border: none;";
        html_response += u8"  background-color: #FFFFFF";
        html_response += u8"}";
        html_response += u8".frm {";
        html_response += u8"  width: auto;";
        html_response += u8"}";
        html_response += u8"</style>";
        html_response += u8"</header>";
        html_response += u8"<script>";
        html_response += u8"window.onload = function() {";
        html_response += u8"    var urlParams = new URLSearchParams(window.location.search);";
        html_response += u8"    if (urlParams.has('fileUploaded') && urlParams.get('fileUploaded') == 'true') {";
        html_response += u8"        alert('Файл успешно загружен!');";
        html_response += u8"    }";
        html_response += u8"};";
        html_response += u8"</script>";
        html_response += u8"<body><div><h1>Таблица пользователей</h1>";
        html_response += u8"<table><tr><td><strong>Имя</strong></td><td><strong>Группа</strong></td><td><strong>Роль</strong></td></tr>";

        for (const auto& user : users)
        {
            html_response += u8"<tr>";
            html_response += u8"<td>" + user.name + "</td>";
            html_response += u8"<td>" + user.group + "</td>";
            html_response += u8"<td class='frm'>";
            html_response += u8"<form action='/update-role' method='post'>";
            html_response += u8"<input class='inp' type='text' name='role' value='" + user.role + "'>";
            html_response += u8"<input type='hidden' name='username' value='" + user.name + "'>";
            html_response += u8"</form>";
            html_response += u8"<form action='/delete-user' method='post'>";
            html_response += u8"<input type='hidden' name='username' value='" + user.name + "'>";
            html_response += u8"<button class='delete-button' type='submit'>Удалить</button>";
            html_response += u8"</form>";
            html_response += u8"</td></tr>";
        }

        html_response += u8"</table>";
        html_response += u8"<br><br>";
        html_response += u8"<form action='/logout' method='get'>";
        html_response += u8"<button class='logout-button' type='submit'>Выход</button>";
        html_response += u8"</form>";
        html_response += u8"</div>";
        html_response += u8"<div>";
        html_response += u8"<form action='/upload-file' method='post' enctype='multipart/form-data'>";
        html_response += u8"<input type='file' name='file' accept='.xlsx'>";
        html_response += u8"<input type='submit'>";
        html_response += u8"</form>";
        html_response += u8"<form action='/send-file' method='post'>";
        html_response += u8"<button type='sumbit'>Отправить данные</button>";
        html_response += u8"</form>";
        html_response += u8"</div>";
        html_response += u8"</body>";
        html_response += u8"</html>";

        res.set_content(html_response, "text/html");
    }
}

void logoutHandler(const Request& req, Response& res)
{
    if (req.method == "GET")
    {
        auto cookie = Cookie::get_cookie(req, "user_cookie");

        std::cout << "Cookie: " << cookie.value << " now is outdated." << std::endl;

        sessions.erase(cookie.value);

        Cookie new_cookie;
        new_cookie.name = "user_cookie";
        new_cookie.value = "";
        new_cookie.maxAge = -1;

        Cookie::set_cookie(res, new_cookie);
        
        res.set_redirect("/login");
    }
}

void updateRoleHandler(const Request& req, Response& res) 
{
    std::string username = req.get_param_value("username");
    std::string newRole = req.get_param_value("role");

    for (auto& user : users) 
    {
        if (user.name == username) 
        {
            user.role = newRole;
            break;
        }
    }

    bsoncxx::builder::stream::document filter_builder{};
    filter_builder << "Name" << username;

    bsoncxx::builder::stream::document update_builder{};
    update_builder << "$set" << bsoncxx::builder::stream::open_document
        << "Role" << newRole
        << bsoncxx::builder::stream::close_document;

    collection.update_one(filter_builder.view(), update_builder.view());

    res.status = 200;
    res.set_redirect("/");
}

void deleteUserFromVector(const std::string& username) 
{
    users.erase(std::remove_if(users.begin(), users.end(), 
        [&username](const User& user) { return user.name == username; }), users.end());
}

void deleteUserHandler(const Request& req, Response& res)
{
    if (req.method == "POST")
    {
        std::string username = req.get_param_value("username");

        document builder{};
        builder << "Name" << username;

        bsoncxx::stdx::optional<mongocxx::result::delete_result> is_deleted = collection.delete_one(builder.view());

        if (is_deleted)
        {
            deleteUserFromVector(username);
            std::cout << "User " + username + " is successfully deleted!" << std::endl;
        }
        else
        {
            std::cout << "An error occured while deleting a user..." << std::endl;
        }

        res.status = 200;
        res.set_redirect("/");
    }
}

void send_file_to_server(const std::string& server_path, const std::string& file_path) 
{
    httplib::Client cli("26.38.28.245", 8090);

    std::ifstream input_file(file_path, std::ios::binary);
    std::vector<char> file_data((std::istreambuf_iterator<char>(input_file)), std::istreambuf_iterator<char>());

    httplib::MultipartFormDataItems items = 
    {
        { "file", file_data.data(), file_path, "application/octet-stream" },
    };

    auto res = cli.Post(server_path.c_str(), items);

    if (res && res->status == 200) 
    {
        std::cout << "File sent successfully\n";
    }
    else 
    {
        std::cout << "Failed to send file\n";
    }
}

void uploadFileHandler(const Request& req, Response& res)
{
    if (req.method == "POST")
    {
        bool is_file_uploaded = req.has_file("file");

        if (is_file_uploaded)
        {
            std::cout << "Someone just uploaded a file!" << std::endl;
        }
        else
        {
            std::cout << "An error occured while uploading a file...";
            return;
        }

        const auto& file = req.get_file_value("file");
        std::string upload_directory = "C:\\Users\\maxim\\PycharmProjects\\excel_parser";
        std::string path = upload_directory + "\\schedule.xlsx";

        std::ofstream ofs(path, std::ios::binary);
        ofs << file.content;

        _chdir("C:\\Users\\maxim\\PycharmProjects\\excel_parser\\");
        system("start main.exe");

        res.set_redirect("/?fileUploaded=true");
    }
}

void sendFileHandler(const Request& req, Response& res)
{
    if (req.method == "POST")
    {
        send_file_to_server("/upload", "C:\\Users\\maxim\\PycharmProjects\\excel_parser\\output.json");
        res.set_redirect("/");
    }
}
