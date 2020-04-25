#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <termios.h>
#include <unistd.h>

#include "macro_hack.hpp"

int create_socket(const char *ip, int port) {
    int s;
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    s = socket(AF_INET, SOCK_STREAM, 0);

    if (s < 0) {
        fprintf(stderr, "Unable to create socket\n");
        exit(EXIT_FAILURE);
    }

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "Unable to bind\n");
        exit(EXIT_FAILURE);
    }

    if (listen(s, 1) < 0) {
        fprintf(stderr, "Unable to listen\n");
        exit(EXIT_FAILURE);
    }

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
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = TLSv1_2_server_method();
    //method = SSLv23_server_method();

    ctx = SSL_CTX_new(method);
    if (!ctx) {
        fprintf(stderr, "Unable to create SSL context\n");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return ctx;
}

void configure_context(SSL_CTX *ctx) {
    SSL_CTX_set_ecdh_auto(ctx, 1);

    /* Set the key and cert */
    if (SSL_CTX_use_certificate_file(ctx, "test/host.crt", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, "test/host.key", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
}

void pty_child(int pty_slave) {
    struct termios slave_orig_term_settings;  // Saved terminal settings
    struct termios new_term_settings;         // Current terminal settings

    // Save the defaults parameters of the slave side of the PTY
    tcgetattr(pty_slave, &slave_orig_term_settings);

    // Set RAW mode on slave side of PTY
    //new_term_settings = slave_orig_term_settings;
    //cfmakeraw(&new_term_settings);
    //tcsetattr(pty_slave, TCSANOW, &new_term_settings);

    // The slave side of the PTY becomes the standard input and outputs of the child process
    dup2(pty_slave, 0);
    dup2(pty_slave, 1);
    dup2(pty_slave, 2);

    // Now the original file descriptor is useless
    close(pty_slave);

    // Make the current process a new session leader
    setsid();

    // As the child is a session leader, set the controlling terminal to be the slave side of the PTY
    // (Mandatory for programs like the shell to make them manage correctly their outputs)
    ioctl(0, TIOCSCTTY, 1);

    //printf("bash start\n");
    DEFINE_EC(status, execl("/bin/bash", "/bin/bash", NULL), < 0, exit(1));
}

void pty_parent(int pty_master, SSL *ssl, int client_fd) {
    fd_set fd_in;

    char buf[4096] = {0};

    while (true) {
        // Wait for data from standard input and master side of PTY
        FD_ZERO(&fd_in);
        FD_SET(pty_master, &fd_in);
        FD_SET(client_fd, &fd_in);

        int status = select(max(client_fd, pty_master) + 1, &fd_in, NULL, NULL, NULL);

        switch (status) {
            case -1:
                fprintf(stderr, "Error %d on select()\n", errno);
                return;

            default: {
                if (FD_ISSET(client_fd, &fd_in)) {
                    printf("Recv..\n");
                    DEFINE_EC(rev_len, SSL_read(ssl, buf, sizeof(buf)), <= 0, return );
                    write(pty_master, buf, rev_len);
                }
                if (FD_ISSET(pty_master, &fd_in)) {
                    printf("Send..\n");
                    DEFINE_EC(rev_len, read(pty_master, buf, sizeof(buf)), <= 0, return );
                    SSL_write(ssl, buf, rev_len);
                }
            }
        }  // End switch
    }      // End while
}

void pty(SSL *ssl, int client_fd) {
    DEFINE_EC(pty_master, posix_openpt(O_RDWR), < 0, exit(1));

    DEFINE_EC(state, grantpt(pty_master), < 0, exit(1));

    ASSIGN_EC(state, unlockpt(pty_master), < 0, exit(1));

    DEFINE_EC(pty_slave, open(ptsname(pty_master), O_RDWR), < 0, exit(1));

    DEFINE_EC(pid, fork(), < 0, );

    if (pid == 0) {
        printf("slave started\n");
        close(pty_master);
        pty_child(pty_slave);
    }
    if (pid > 0) {
        printf("master started\n");
        close(pty_slave);
        pty_parent(pty_master, ssl, client_fd);
    }
}

int main(int argc, char **argv) {
    if (argc <= 2) {
        fprintf(stderr, "Try: ./server [ip] [port]");
        exit(1);
    }

    int sock;
    SSL_CTX *ctx;
    init_openssl();
    ctx = create_context();
    configure_context(ctx);

    sock = create_socket(argv[1], atoi(argv[2]));
    char buf[4096];
    /* Handle connections */
    while (1) {
        struct sockaddr_in addr;
        uint len = sizeof(addr);
        SSL *ssl;

        printf("connected..\n");

        DEFINE_EC(client, accept(sock, (struct sockaddr *)&addr, &len), < 0, exit(1));

        ssl = SSL_new(ctx);

        SSL_set_fd(ssl, client);

        printf("waiting..\n");

        DEFINE_EC(status, SSL_accept(ssl), < 0, continue);

        printf("accepted\n");

        pty(ssl, client);

        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(client);
    }

    close(sock);
    SSL_CTX_free(ctx);
    cleanup_openssl();
}
