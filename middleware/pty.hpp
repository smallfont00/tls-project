#pragma once

#include <map>
#include <string>

#include "../utils/Request.hpp"
#include "../utils/Response.hpp"
#include "../utils/Selector.hpp"
#include "../utils/linux_dependency.hpp"

class PTY {
   public:
    Selector selector;
    int master;
    PTY() {
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
            master = pty_master;
        }
    }

   private:
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
};