#pragma once

#include <filesystem>
#include <regex>
#include <string>

#include "global_define.hpp"

namespace fs = std::filesystem;

class SimpleHttpParser {
   private:
   public:
    Message parser(std::string str) {
        const std::regex DOUBLE_LINE("\r\n\r\n");
        const std::regex SINGLE_LINE("\r\n");
        const std::regex SPACE("\\s+");
        const std::regex QUESTION("\\?");
        Message msg;
        std::vector<std::string> v1(std::sregex_token_iterator(str.begin(), str.end(), DOUBLE_LINE, -1),
                                    std::sregex_token_iterator());

        if (v1[0] == "") {
            return msg;
        }

        std::vector<std::string> v2(std::sregex_token_iterator(v1[0].begin(), v1[0].end(), SINGLE_LINE, -1),
                                    std::sregex_token_iterator());

        std::vector<std::string> v3(std::sregex_token_iterator(v2[0].begin(), v2[0].end(), SPACE, -1),
                                    std::sregex_token_iterator());

        std::vector<std::string> v4(std::sregex_token_iterator(v3[1].begin(), v3[1].end(), QUESTION, -1),
                                    std::sregex_token_iterator());

        msg["Query"] = "";
        msg["Body"] = "";
        msg["Method"] = v3[0];

        msg["Path"] = v4[0];

        if (v4.size() == 2) {
            msg["Query"] = v4[1];
        }

        if (v1.size() == 2) {
            msg["Body"] = v1[1];
        }

        for (int i = 1; i < v2.size(); i++) {
            auto offset = v2[i].find_first_of(':');
            auto first = v2[i].substr(0, offset);
            auto second = v2[i].substr(offset + 2, v2[i].size() - offset);
            msg[first] = second;
        }
        return msg;
    }
};