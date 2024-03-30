#ifndef HTTPD_H
#define HTTPD_H

#include <iostream>
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <memory>
#include <queue>
#include <mutex>
#include <thread>
#include <functional>
#include <future>
#include <type_traits>
#include <atomic>
#include <chrono>
#include <semaphore.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include "utils.h"

#define MAX_LISTEN_QUEUE_LEN 6
#define READ_TIMEOUT_SEC 5

namespace httpd
{

namespace utils
{
// 字符串转小写
const std::string toLower(const std::string& str);

// URL解码
const std::shared_ptr<std::string> urlDecode(const std::shared_ptr<std::string> input);

// URL编码
const std::shared_ptr<std::string> urlEncode(const std::shared_ptr<std::string> input);

} // namespace utils


/*------------Definition of MessageQueue--------------*/
class Method{ //方法类
public:
    enum class Type{ //方法的类型
        GET,
        POST
    };
    Method(const Method::Type& type);
    Method(const std::shared_ptr<std::string> str);

    Method::Type getType() const;
    std::shared_ptr<std::string> toString() const;
    bool operator==(const Method& cmp) const;
    bool operator==(const Method::Type& cmp) const;
    bool operator!=(const Method& cmp) const;
    bool operator!=(const Method::Type& cmp) const;
private:
    Method::Type type;
};

/*------------Definition of Version--------------*/
class Version{ //版本类
public:
    enum class Type{ //两个Http版本
        HTTP_1_0,
        HTTP_1_1
    };
    Version(const Version::Type& type);
    Version(const std::shared_ptr<std::string> str);

    Version::Type getType() const;
    std::shared_ptr<std::string> toString() const;
    bool operator==(const Version& cmp) const;
    bool operator==(const Version::Type& cmp) const;
    bool operator!=(const Version& cmp) const;
    bool operator!=(const Version::Type& cmp) const;
private:
    Version::Type type;
};

/*------------Definition of StatusCodeAndMessage--------------*/
class StatusCodeAndMessage{ //状态码类
public:
    enum class Type{ //状态码
        UNKNOW=0,
        Continue = 100,
        OK = 200,
        BadRequest = 400,
        Unauthorized = 401,
        Forbidden = 403,
        NotFound = 404,
        InternalServerError = 500
    };
    StatusCodeAndMessage(const StatusCodeAndMessage::Type& type);

    StatusCodeAndMessage::Type getType() const;
    std::shared_ptr<std::string> toString() const;
    bool operator==(const StatusCodeAndMessage& cmp) const;
    bool operator==(const StatusCodeAndMessage::Type& cmp) const;
    bool operator!=(const StatusCodeAndMessage& cmp) const;
    bool operator!=(const StatusCodeAndMessage::Type& cmp) const;

private:
    StatusCodeAndMessage::Type type;
};

/*------------Definition of Body--------------*/
class Body{ //请求体类
public:
    Body(const std::shared_ptr<std::string> type, const std::shared_ptr<std::vector<unsigned char>> content);
    const std::shared_ptr<std::string> getType() const; //获取内容的Content-Type
    const std::shared_ptr<std::vector<unsigned char>> getContent() const; //获取Body的数据

private:
    std::shared_ptr<std::string> sp_type;
    std::shared_ptr<std::vector<unsigned char>> sp_content;
};

/*------------Definition of HttpException--------------*/
class HttpException : public std::exception { // 异常类
public:
    HttpException(const StatusCodeAndMessage::Type& type);
    virtual const char* what() const throw();

    const std::shared_ptr<StatusCodeAndMessage> getStatusCodeAndMessage() const;

private:
    const std::shared_ptr<StatusCodeAndMessage> sp_status_code_and_msg;
};

/*------------Definition of Request--------------*/
class Request { //请求类用于表示HTTP请求
public:
    Request();

    void decode(const std::shared_ptr<std::string> str); // 将字符串解析为Request对象
    const std::shared_ptr<std::string> encode(); //将Request对象编码为字符串

    void setMethod(const Method::Type& type);
    const std::shared_ptr<Method> getMethod() const;
    void setPath(const std::shared_ptr<std::string> path);
    const std::shared_ptr<std::string> getPath() const;
    void setVersion(const Version::Type& type);
    const std::shared_ptr<Version> getVersion() const;
    void setHeader(const std::shared_ptr<std::string> key,const std::shared_ptr<std::string> value);
    const std::shared_ptr<std::string> getHeader(const std::shared_ptr<std::string> key) const;
    void setBody(const std::shared_ptr<Body> body);
    const std::shared_ptr<Body> getBody() const;

private:
    static std::tuple<std::string,std::string,std::string> parseInitiaLine(const std::string& line); //解析http请求的初始化
    static std::string trimWhitespace(const std::string& str); //消除前导和后导不可见字符
    static std::pair<std::string,std::string> parseKeyValuePairLine(const std::string& line); //解析一行键值对

private:
    std::shared_ptr<Method> sp_method;
    std::shared_ptr<std::string> sp_path;
    std::shared_ptr<Version> sp_version;
    std::shared_ptr<std::unordered_map<std::string,std::shared_ptr<std::string>>> sp_headers;
    std::shared_ptr<Body> sp_body;

};

/*------------Definition of Response--------------*/
class Response { //响应类用于表示HTTP响应
public:
    Response();

    const std::shared_ptr<std::string> encode(); //将Response对象编码为字符串

    void setVersion(const Version& version);
    const std::shared_ptr<Version> getVersion() const;
    void setStatusCodeAndMessage(const std::shared_ptr<StatusCodeAndMessage> status_code_and_msg);
    const std::shared_ptr<StatusCodeAndMessage> getStatusCodeAndMessage() const;
    void setHeader(const std::shared_ptr<std::string> key,const std::shared_ptr<std::string> value);
    const std::shared_ptr<std::string> getHeader(const std::shared_ptr<std::string> key) const;
    void setBody(const std::shared_ptr<Body> body);
    const std::shared_ptr<Body> getBody() const;

public:
    static std::shared_ptr<Response> quickBuild(const std::shared_ptr<StatusCodeAndMessage> status_code_and_msg); //用状态码快速构建一个Response对象

private:
    std::shared_ptr<Version> sp_version;
    std::shared_ptr<StatusCodeAndMessage> sp_status_code_and_msg;
    std::shared_ptr<std::unordered_map<std::string,std::shared_ptr<std::string>>> sp_headers;
    std::shared_ptr<Body> sp_body;
    
};

/*------------Definition of FileSystem--------------*/
class FileSystem{ //文件系统
public:
    FileSystem(const std::shared_ptr<std::string> file_root);
    
    const std::shared_ptr<Body> read(const std::shared_ptr<std::string> file_name); //读取文件的内容包装成一个Body

private:
    bool isAccessPermitted(const std::shared_ptr<std::string> file_name) const; //判断是否escape文件目录

private:
    std::shared_ptr<std::string> sp_file_root;
    std::unordered_map<std::string,std::string> mime_types{
        {"css", "text/css"},
        {"csv", "text/csv"},
        {"txt", "text/plain"},
        {"vtt", "text/vtt"},
        {"html", "text/html"},
        {"htm", "text/html"},
        {"apng", "image/apng"},
        {"avif", "image/avif"},
        {"bmp", "image/bmp"},
        {"gif", "image/gif"},
        {"png", "image/png"},
        {"svg", "image/svg+xml"},
        {"webp", "image/webp"},
        {"ico", "image/x-icon"},
        {"tif", "image/tiff"},
        {"tiff", "image/tiff"},
        {"jpeg", "image/jpeg"},
        {"jpg", "image/jpeg"},
        {"mp4", "video/mp4"},
        {"mpeg", "video/mpeg"},
        {"webm", "video/webm"},
        {"mp3", "audio/mp3"},
        {"mpga", "audio/mpeg"},
        {"weba", "audio/webm"},
        {"wav", "audio/wave"},
        {"otf", "font/otf"},
        {"ttf", "font/ttf"},
        {"woff", "font/woff"},
        {"woff2", "font/woff2"},
        {"7z", "application/x-7z-compressed"},
        {"atom", "application/atom+xml"},
        {"pdf", "application/pdf"},
        {"mjs", "application/javascript"},
        {"js", "application/javascript"},
        {"json", "application/json"},
        {"rss", "application/rss+xml"},
        {"tar", "application/x-tar"},
        {"xhtml", "application/xhtml+xml"},
        {"xht", "application/xhtml+xml"},
        {"xslt", "application/xslt+xml"},
        {"xml", "application/xml"},
        {"gz", "application/gzip"},
        {"zip", "application/zip"},
        {"wasm", "application/wasm"}
    };
};

/*------------Definition of Rule--------------*/
class Rule{ //规则类
public:
    Rule(const std::shared_ptr<std::string> sp_rule_str);
    bool isAllow() const; //判断基于该规则是否允许访问
    bool isMatch(const std::shared_ptr<std::string> sp_ip) const; //判断该IP是否匹配到该规则

private:
    enum class Action{
        ALLOW,DENY
    };
    struct IPNetwork {
        uint32_t network;
        uint32_t mask;
    };

    IPNetwork network;
    Action action;
};

/*------------Definition of IPAccessControl--------------*/
class IPAccessControl { //IP访问控制类
public:
    IPAccessControl(const std::shared_ptr<std::string> sp_rule_file);
    bool isAllow(const std::shared_ptr<std::string> sp_ip) const; //判断该IP是否可以访问

private:
    std::vector<Rule> rules;
};

/*------------Definition of Server--------------*/
class Server{ //服务类
public:
    Server(const int port, const size_t pool_size ,const std::shared_ptr<std::string> sp_rule_file);
    ~Server();

    void setMessageCallback(std::function<std::shared_ptr<httpd::Response>(const std::shared_ptr<httpd::Request>)> callback); //设置一个消息回调函数
    void run(); //服务运行

private:
    void task(int client_fd);

private:
    int server_fd;
    std::shared_ptr<IPAccessControl> sp_ip_access_control;
    std::shared_ptr<::utils::ThreadPool> sp_pool;
    std::function<std::shared_ptr<httpd::Response>(const std::shared_ptr<httpd::Request>)> message_callback;
};


} // namespace httpd

void start_httpd(unsigned short port, std::string doc_root, size_t pool_size=6);

#endif // HTTPD_H