#pragma once

#include <string>

#include "../utils/Request.hpp"
#include "../utils/Response.hpp"

class CGI_Handler {
   public:
    void operator()(Request<> &req, Response &res) {
        if (req.content_type() == "application/cgi") {
            std::cout << "cgi >>" << std::endl;
            res.status(200).send(cgi_handler(req.path(), req.query(), req.body()));
        }
    }

   private:
    std::string cgi_handler(std::string path, std::string query = "", std::string body = "") {
        int cgiInput[2];
        int cgiOutput[2];
        int status;
        pid_t cpid;
        char c;

        char buffer[4096] = {0};

        if (pipe(cgiInput) < 0) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
        if (pipe(cgiOutput) < 0) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        if ((cpid = fork()) < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        /*child process*/
        if (cpid == 0) {
            close(cgiInput[1]);
            close(cgiOutput[0]);

            dup2(cgiOutput[1], STDOUT_FILENO);

            dup2(cgiInput[0], STDIN_FILENO);

            close(cgiInput[0]);
            close(cgiOutput[1]);

            execlp(path.c_str(), path.c_str(), NULL);
            exit(0);
        }

        /*parent process*/
        else {
            std::string input = "";
            if (query != "")
                input += query + "\n";
            if (body != "")
                input += body + "\n";

            //close unused fd
            close(cgiOutput[1]);
            close(cgiInput[0]);

            // send the message to the CGI program

            write(cgiInput[1], input.c_str(), input.size());

            read(cgiOutput[0], buffer, sizeof(buffer));

            // connection finish
            close(cgiOutput[0]);
            close(cgiInput[1]);

            waitpid(cpid, &status, 0);

            return std::string(buffer);
        }
    }
};
