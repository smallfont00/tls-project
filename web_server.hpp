#include <algorithm>
#include <chrono>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

#include "utils/Request.hpp"
#include "utils/Response.hpp"
#include "utils/Selector.hpp"
#include "utils/blob_reader.hpp"
#include "utils/linux_dependency.hpp"
using namespace std;

class Server {
   private:
    int socket_ = -1;
    SSL_CTX *ctx_ = NULL;
    std::vector<std::pair<std::string, function<void(Request<> &, Response &)>>> actions;
    char buf[4096];

   public:
    Selector selector;
    void Use(
        std::string path, std::function<void(Request<> &, Response &)> action = [](Request<> &, Response &) {}) {
        if ((!path.empty()) && (path[0] == '/')) path = path.substr(1);
        actions.push_back(make_pair(path, action));
    }

    void Use(std::function<void(Request<> &, Response &)> action = [](Request<> &, Response &) {}) {
        Use("/*", action);
    }

    int Listen(std::string port, std::function<void()> action) {
        init_openssl();
        ctx_ = create_context();
        configure_context(ctx_);
        socket_ = create_socket(std::stoi(port));

        action();

        selector.subscribe(socket_, [&]() {
            struct sockaddr_in addr;
            uint len = sizeof(addr);
            DEFINE_EC(client_socket, accept(socket_, (struct sockaddr *)&addr, &len), < 0, return;);
            SSL *ssl = SSL_new(ctx_);
            SSL_set_fd(ssl, client_socket);
            std::cout << "[new socket] " << client_socket << std::endl;
            selector.subscribe(client_socket, [&, client_socket, ssl]() {
                std::cout << "[receiving] " << client_socket << std::endl;
                receive_action(client_socket, ssl);
            });
        });

        while (true) {
            selector.active();
        }

        close(this->socket_);

        return 0;
    }

   private:
    void receive_action(int client_socket, SSL *ssl) {
        DEFINE_EC(status, SSL_accept(ssl), != 1, selector.close_ssl(client_socket, ssl); return;);

        auto req = Request<>();
        auto res = Response(client_socket, ssl);

        DEFINE_EC(rev_len, SSL_read(ssl, buf, sizeof(buf)), <= 0, selector.close_ssl(client_socket, ssl); return;);

        req.parse(buf, rev_len);
        for (auto &[path, action] : actions) {
            if ((path == "*") || (path == req.path())) {
                action(req, res);
            }
        }
        //SSL_shutdown(ssl);
        //SSL_free(ssl);
        //close(client_socket);
    }

    int create_socket(int port) {
        struct sockaddr_in addr;

        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);

        DEFINE_EC(s, socket(AF_INET, SOCK_STREAM, 0), < 0, exit(1));

        ERR_CHECK(bind(s, (struct sockaddr *)&addr, sizeof(addr)), < 0, exit(1));

        ERR_CHECK(listen(s, 1), < 0, exit(1));

        return s;
    }

    void init_openssl() {
        SSL_load_error_strings();
        OpenSSL_add_ssl_algorithms();
    }

    void cleanup_openssl() {
        EVP_cleanup();
    }

    SSL_CTX *create_context() {
        const SSL_METHOD *method = TLS_server_method();
        //const SSL_METHOD *method = SSLv23_server_method();
        DEFINE_EC(ctx, SSL_CTX_new(method), == NULL, exit(1));

        return ctx;
    }

    void configure_context(SSL_CTX *ctx) {
        SSL_CTX_set_ecdh_auto(ctx, 1);

        ERR_CHECK(SSL_CTX_load_verify_locations(ctx, "hw3_key/root_cert.pem", NULL), == 0, exit(1));

        ERR_CHECK(SSL_CTX_use_certificate_file(ctx, "hw3_key/B10615004.crt", SSL_FILETYPE_PEM), <= 0, exit(1));

        ERR_CHECK(SSL_CTX_use_PrivateKey_file(ctx, "hw3_key/B10615004.key", SSL_FILETYPE_PEM), <= 0, exit(1));
    }
};
