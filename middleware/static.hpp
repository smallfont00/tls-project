#pragma once

#include <filesystem>
#include <iostream>
#include <string>

#include "../utils/Request.hpp"
#include "../utils/Response.hpp"
#include "../utils/blob_reader.hpp"

namespace fs = std::filesystem;

class Static {
   private:
    std::string host_path;

   public:
    Static(std::string path = "") : host_path(path) {
        if (host_path.empty()) {
            this->host_path = "";
            return;
        }
        if (this->host_path.back() != '/') {
            this->host_path.push_back('/');
            return;
        }
    }
    void operator()(Request<> &req, Response &res) {
        if (req.path() == "") {
            req.path() = "index.html";
        }

        std::error_code target_error;
        auto relative_path = fs::path(host_path) / req.path();

        auto static_path = fs::canonical(host_path);
        auto target_path = fs::canonical(relative_path, target_error);

        auto extension = relative_path.extension().string();

        std::string content_type = "text/html";
        std::string body = "";

        if (extension == ".jpg") {
            content_type = "image/jpeg";
        } else if (extension == ".html") {
            content_type = "text/html";
        } else if (extension == ".cgi") {
            req.content_type() = "application/cgi";
            req.path() = relative_path;
            return;
        } else if (extension == ".js") {
            content_type = "text/javascript";
        } else if (extension == ".css") {
            content_type = "text/css";
        }

        if (target_error.value()) {
            if (req["Accept"] == "*/*") return;
            std::cerr << target_error.message() << " : " << req.path() << std::endl;
            if (extension != "") res.status(404).send("");
            return;
        }

        if (target_path.string().find_first_of(static_path.string()) != 0) {
            std::cerr << "outside the static path" << std::endl;
            //res.status(404).send("");
            return;
        }

        body = read_file(target_path);

        res.status(200).content_type(content_type).send(body);
    }
};