#include "middleware/cgi_handler.hpp"
#include "middleware/static.hpp"
#include "utils/base64.hpp"
#include "web_server.hpp"
map<std::string, std::string> parse_config(std::string filename) {
    char key[128], value[128], temp[256];
    map<std::string, std::string> result;
    ifstream config("server.cfg");
    while (config >> temp) {
        sscanf(temp, "%[^=]=%s", key, value);
        result[key] = value;
        cout << key << " : " << value << endl;
    }
    return result;
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

int pty() {
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
        return pty_master;
    }
    return pty_master;
}

int main() {
    auto cfg = parse_config("server.cfg");

    Server app;

    app.Use(Static(cfg["STATIC_PATH"]));

    app.Use(CGI_Handler());

    app.Use("/api/v1/demo", [&](Request<> &req, Response &res) {
        static bool is_on = 0;
        std::cout << "[demo] " << req.body().size() << " " << req.body() << std::endl;
        res.status(202).send("");
    });

    int pty_master = pty();

    unsigned char buf[4096] = {0};

    app.Use("/api/v1/pty", [&](Request<> &req, Response &res) {
        app.selector.subscribe(pty_master, [&, res]() {
            DEFINE_EC(rev_len, read(pty_master, buf, sizeof(buf)), <= 0, app.selector.unsubscribe(res.fd, res.ssl); return;);
            auto m = base64_encode(buf, rev_len);
            std::cout << "[pty >> ] " << m << std::endl;
            res.write("data: " + m + "\n\n");
        });
        res.status(200).content_type("text/event-stream").writeHeader();
    });

    app.Use("/api/v1/pty/write", [&](Request<> &req, Response &res) {
        std::cout << "[pty << ] " << req.body() << std::endl;
        write(pty_master, req.body().c_str(), req.body().size());
        res.status(202).send("");
    });

    app.Listen(cfg["PORT"], []() -> void {
        cout << "Server started" << endl;
    });
}