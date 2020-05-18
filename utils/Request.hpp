#pragma once

#include <iostream>

#include "SimpleHttpParser.hpp"

template <typename HttpParserType = SimpleHttpParser>
class Request : HttpParserType {
   private:
    char buf[4096];
    Message msg;

   public:
    std::string &path() {
        return msg["Path"];
    }
    std::string &query() {
        return msg["Query"];
    }
    std::string &body() {
        return msg["Body"];
    }
    std::string &content_type() {
        return msg["Content_Type"];
    }

    void parse(char *buf, size_t size) {
        buf[size] = 0;
        msg = HttpParserType::parser(buf);
        msg["Path"] = msg["Path"].substr(1);
    }

    std::string &operator[](const std::string &key) {
        return msg[key];
    }
};