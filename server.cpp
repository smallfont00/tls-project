#include "middleware/static.hpp"
#include "utils/linux_dependency.hpp"

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
    //const SSL_METHOD *method = TLS_server_method();
    const SSL_METHOD *method = SSLv23_server_method();
    DEFINE_EC(ctx, SSL_CTX_new(method), == NULL, exit(1));

    return ctx;
}

void configure_context(SSL_CTX *ctx) {
    SSL_CTX_set_ecdh_auto(ctx, 1);

    //SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT | SSL_VERIFY_CLIENT_ONCE, NULL);

    //DEFINE_EC(CA_cert, SSL_load_client_CA_file("CA_key/CA.crt"), == NULL, exit(1));

    //SSL_CTX_set_client_CA_list(ctx, CA_cert);

    ERR_CHECK(SSL_CTX_load_verify_locations(ctx, "hw3_key/root_cert.pem", NULL), == 0, exit(1));

    //ERR_CHECK(SSL_CTX_use_certificate_chain_file(ctx, "CA_key/CA.crt"), <= 0, exit(1));

    //SSL_CTX_set_verify_depth(ctx, 1);

    ERR_CHECK(SSL_CTX_use_certificate_file(ctx, "hw3_key/B10615004.crt", SSL_FILETYPE_PEM), <= 0, exit(1));

    ERR_CHECK(SSL_CTX_use_PrivateKey_file(ctx, "hw3_key/B10615004.key", SSL_FILETYPE_PEM), <= 0, exit(1));

    //ERR_CHECK(SSL_CTX_check_private_key(ctx), == 0, exit(1));
}

void pty_child(int pty_slave) {
    dup2(pty_slave, 0);
    dup2(pty_slave, 1);
    dup2(pty_slave, 2);

    termios old, current;

    tcgetattr(0, &old);

    cfmakeraw(&current);

    close(pty_slave);

    // Make the current process a new session leader
    setsid();

    // As the child is a session leader, set the controlling terminal to be the slave side of the PTY
    // (Mandatory for programs like the shell to make them manage correctly their outputs)
    ioctl(0, TIOCSCTTY, 1);

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

        int status = select(std::max(client_fd, pty_master) + 1, &fd_in, NULL, NULL, NULL);

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
        }
    }
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
        kill(pid, SIGKILL);
    }
}

static int sock = -1, client = -1;
static SSL_CTX *ctx = NULL;

void signal_callback_handler(int signum) {
    printf("\nServer is closing\n");
    if (client >= 0) close(client);
    if (sock >= 0) close(sock);
    if (ctx) SSL_CTX_free(ctx);
    cleanup_openssl();
    printf("bye~\n");
    exit(1);
}

int main(int argc, char **argv) {
    if (argc <= 1) {
        fprintf(stderr, "Try: ./server [port]\n");
        exit(1);
    }
    signal(SIGINT, signal_callback_handler);

    init_openssl();

    ctx = create_context();

    configure_context(ctx);

    sock = create_socket(atoi(argv[1]));

    char buf[4096];

    /* Handle connections */
    while (true) {
        struct sockaddr_in addr;
        uint len = sizeof(addr);

        printf("connecting..\n");

        ASSIGN_EC(client, accept(sock, (struct sockaddr *)&addr, &len), < 0, exit(1));

        SSL *ssl = SSL_new(ctx);
        SSL_set_fd(ssl, client);

        printf("waiting..\n");

        DEFINE_EC(status, SSL_accept(ssl), != 1, continue);

        // DEFINE_EC(peerCertificate, SSL_get_peer_certificate(ssl), == NULL, exit(1));

        // char commonName[512];

        // X509_NAME *name = X509_get_subject_name(peerCertificate);

        // X509_NAME_get_text_by_NID(name, NID_commonName, commonName, 512);

        // printf("Hostname: %s\n", commonName);

        // printf("Connected with %s encryption\n", SSL_get_cipher(ssl));

        // printf("accepted, pty start..\n");

        pty(ssl, client);

        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(client);
    }

    close(sock);
    SSL_CTX_free(ctx);
    cleanup_openssl();
}
