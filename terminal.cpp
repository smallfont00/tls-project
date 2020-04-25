#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
int main() {
    int pipefds[2];
    if (pipe(pipefds) < 0) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        close(pipefds[1]);

        int output_fd = open("terminal_out", O_WRONLY | O_CREAT | O_TRUNC, 0777);
        if (output_fd < 0) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        if (dup2(pipefds[0], STDIN_FILENO) < 0) {
            perror("dup2 (stdin)");
            exit(EXIT_FAILURE);
        }

        /*
        if (dup2(output_fd, STDOUT_FILENO) < 0) {
            perror("dup2 (stdout)");
            exit(EXIT_FAILURE);
        }*/
        perror("4");
        if (execlp("/bin/bash", "/bin/bash", NULL) < 0) {
            perror("execl");
            exit(EXIT_FAILURE);
        }
    } else {
        close(pipefds[0]);
        //write(pipefds[1], "pwd\n", 4);
        char buf[4096] = {0};
        while (fgets(buf, 4096, stdin) != NULL) {
            //printf("%zu", strlen(buf));
            write(pipefds[1], buf, strlen(buf));
        }

        wait(NULL);
        close(pipefds[1]);
    }
}