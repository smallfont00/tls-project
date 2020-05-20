#pragma once
#include <functional>
#include <map>

#include "linux_dependency.hpp"

class Selector {
   private:
    fd_set fd_in;
    std::map<int, std::function<void()>> fileno;

   public:
    void subscribe(int fd, std::function<void()> action) {
        fileno[fd] = action;
    }
    void unsubscribe(int fd) {
        close(fd);
        fileno.erase(fd);
    }
    void unsubscribe(int fd, SSL *ssl) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(fd);
        fileno.erase(fd);
    }
    void active() {
        if (fileno.empty()) return;
        FD_ZERO(&fd_in);
        for (auto &[fd, action] : fileno) FD_SET(fd, &fd_in);
        int status = select(fileno.rbegin()->first + 1, &fd_in, NULL, NULL, NULL);
        for (auto &[fd, action] : fileno) {
            if (FD_ISSET(fd, &fd_in)) action();
        }
    }
};