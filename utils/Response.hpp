#pragma once

#include <iostream>

#include "linux_dependency.hpp"

class Response {
   private:
    Message msg;

   public:
    int fd;
    SSL *ssl;
    // Response &body(std::string body) {
    //     msg["Content-Length"] = "Content-Length: " + std::to_string(body.size()) + "\n";
    //     msg["Body"] = std::move(body);
    //     return *this;
    // }
    Response(int fd, SSL *ssl) : fd(fd), ssl(ssl) {}
    Response &status(int code) {
        std::string CODE;
        CODE = "HTTP/1.1 " + std::to_string(code) + "\n";
        // switch (code) {
        //     case 200:
        //         CODE = "HTTP/1.1 200 OK\n";
        //         break;
        //     case 404:
        //         CODE = "HTTP/1.1 404 Not Found\n";
        //         break;
        //     default:
        //         CODE = "HTTP/1.1 505 Internal Server Error\n";
        //         break;
        // }
        msg["Code"] = std::move(CODE);
        return *this;
    }
    Response &content_type(std::string type) {
        msg["Content-Type"] = std::move("Content-Type: " + type + "\n");
        return *this;
    }

    Response &connection(bool open) {
        if (open) {
            msg["Connection"] = "Connection: keep-alive\n";
        } else {
            msg["Connection"] = "Connection: close\n";
        }
        return *this;
    }

    Response() {
        msg["Connection"] = "Connection: close\n";
        msg["Content-Type"] = "Content-Type: text/plain\n";
        connection(true);
    };

    void send(const std::string &body) const {
        std::string data = this->message(body);
        SSL_write(ssl, data.c_str(), data.size());
    }

    void writeHeader() const {
        std::string str;
        if (msg.count("Code")) str += msg.at("Code");
        if (msg.count("Content-Type")) str += msg.at("Content-Type");
        if (msg.count("Connection")) str += msg.at("Connection");
        if (str.back() == '\n') str.back() = '\r';
        str += "\n\r\n";
        SSL_write(ssl, str.c_str(), str.size());
    }

    void write(std::string str) const {
        std::string m = str;
        SSL_write(ssl, m.c_str(), m.size());
    }
    void end() {
        //std::string str = "\n\n";
        //SSL_write(ssl, str.c_str(), str.size());
    }

    std::string message(std::string body = "") const {
        std::string str;
        std::string content_length = "Content-Length: " + std::to_string(body.size()) + "\n";
        if (msg.count("Code")) str += msg.at("Code");
        if (msg.count("Content-Type")) str += msg.at("Content-Type");
        if (msg.count("Connection")) str += msg.at("Connection");
        str += content_length;

        if (str.back() == '\n') {
            str.back() = '\r';
            str += "\n\r\n";
        } else {
            str += "\r\n\r\n";
        }

        str += body;

        return str;
    }

    std::string &operator[](const std::string &key) {
        return msg[key];
    }
};