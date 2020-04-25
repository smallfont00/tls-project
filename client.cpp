#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include "macro_hack.hpp"

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int create_socket(const char *addr, const char *port) {
    int sockfd, numbytes;
    char buf[4096];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(addr, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        exit(1);
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);

    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo);
    return sockfd;
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

    method = TLSv1_2_client_method();
    //method = SSLv23_client_method();

    ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return ctx;
}

void configure_context(SSL_CTX *ctx) {
    if (!SSL_CTX_load_verify_locations(ctx, "test/cert.pem", NULL)) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
}

static struct termios old, current;

/* Initialize new terminal i/o settings */
void initTermios(int echo) {
    tcgetattr(0, &old); /* grab old terminal i/o settings */

    current = old;              /* make new settings same as old settings */
    current.c_lflag &= ~ICANON; /* disable buffered i/o */
    if (echo) {
        current.c_lflag |= ECHO; /* set echo mode */
    } else {
        current.c_lflag &= ~ECHO; /* set no echo mode */
    }

    tcsetattr(0, TCSANOW, &current); /* use these new terminal i/o settings now */
}

/* Restore old terminal i/o settings */
void resetTermios(void) {
    tcsetattr(0, TCSANOW, &old);
}

static int sock = -1;
static SSL_CTX *ctx = NULL;

void signal_callback_handler(int signum) {
    printf("\nClient is closing\n");
    if (sock >= 0) close(sock);
    if (ctx) SSL_CTX_free(ctx);
    cleanup_openssl();
    resetTermios();
    printf("bye~\n");
    exit(1);
}

int main(int argc, char **argv) {
    signal(SIGINT, signal_callback_handler);

    init_openssl();

    ctx = create_context();

    configure_context(ctx);

    if (argc <= 2) {
        fprintf(stderr, "Try: ./client [ip] [port]\n");
        exit(1);
    }

    sock = create_socket(argv[1], argv[2]);

    /* Handle connections */
    while (1) {
        SSL *ssl;
        ssl = SSL_new(ctx);
        SSL_set_fd(ssl, sock);

        TEMP_EC(SSL_get_verify_result(ssl), != X509_V_OK, exit(1));

        TEMP_EC(SSL_connect(ssl), != 1, exit(1));

        X509 *peerCertificate = SSL_get_peer_certificate(ssl);

        if (peerCertificate == NULL) {
            printf("No certificate information！\n");
            ERR_print_errors_fp(stderr);
            exit(EXIT_FAILURE);
        }

        char commonName[512];
        X509_NAME *name = X509_get_subject_name(peerCertificate);
        X509_NAME_get_text_by_NID(name, NID_commonName, commonName, 512);

        printf("Hostname: %s\n", commonName);

        printf("Connected with %s encryption\n", SSL_get_cipher(ssl));

        fd_set fd_in;

        printf("sock: %d\n", sock);
        char buf[4096] = {0};

        initTermios(0);

        while (true) {
            FD_ZERO(&fd_in);
            FD_SET(STDIN_FILENO, &fd_in);
            FD_SET(sock, &fd_in);

            int status = select(sock + 1, &fd_in, NULL, NULL, NULL);
            switch (status) {
                case -1:
                    fprintf(stderr, "Error %d on select()\n", errno);
                    return 0;

                default: {
                    if (FD_ISSET(STDIN_FILENO, &fd_in)) {
                        int len = read(STDIN_FILENO, buf, sizeof(buf));
                        DEFINE_EC(rev_len, SSL_write(ssl, buf, len), <= 0, resetTermios(); return 0);
                    }
                    if (FD_ISSET(sock, &fd_in)) {
                        DEFINE_EC(rev_len, SSL_read(ssl, buf, sizeof(buf)), <= 0, resetTermios(); return 0);
                        write(1, buf, rev_len);
                    }
                }
            }  // End switch
        }

        resetTermios();

        printf("end\n");

        int result = SSL_shutdown(ssl);
        result = SSL_shutdown(ssl);
        SSL_free(ssl);
        close(sock);
        break;
    }

    SSL_CTX_free(ctx);
    cleanup_openssl();
}
