#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

volatile sig_atomic_t running = 1;

void sethandler(void (*f)(int), int sigNo) {
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;

    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void handlerAl(int sig) {
    if (sig == SIGALRM) {
        running = 0; // zakończ program
    }
}

void handlerUsr(int sig) {
    if (sig == SIGUSR1) {
    }
}

void child(char n) {
    int op;
    char *buf;
    int count = 0;
    pid_t pid = getpid();
    srand(pid);
    int s = (rand() % 90 + 10) * 1024;
    fprintf(stderr, "[%3d] %5d %c\n", pid, s, n);
    char name[30];
    snprintf(name, sizeof(name), "%d", pid);
    strcat(name, ".txt");
    if ((op = open(name, O_WRONLY | O_CREAT, 0644)) < 0) {
        ERR("open");
    }
    if ((buf = malloc(s)) == NULL) {
        ERR("malloc");
    }
    for (int i = 0; i < s; i++) {
        buf[i] = n;
    }

    sigset_t mask;
    sigfillset(&mask); // blokujemy wszystkie sygnały
    sigdelset(&mask, SIGUSR1); // z wyjątkiem SIGUSR1
    // sigdelset(&mask, SIGALRM); // z wyjątkiem SIGUSR1

    // alarm(1);

    sethandler(handlerAl, SIGALRM);
    sethandler(handlerUsr, SIGUSR1);

    while (running) {
        sigsuspend(&mask);
        count++;
        if (write(op, buf, s) != s) {
            ERR("write");
        }
    }
    fprintf(stderr, "[%3d] sent %d overall\n", pid, count);
    close(op);
    free(buf);
}


void parent() {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &mask, NULL);

    struct timespec t = {0, 10000000};
    alarm(1);
    sethandler(handlerAl, SIGALRM);
    while (running) {
        nanosleep(&t, NULL);
        if (kill(0, SIGUSR1) == -1) {
            ERR("kill");
        }
    }
    // kill(0, SIGALRM);
}


int main(int argc, char *argv[]) {
    pid_t ppid = getpid();

    pid_t pid;
    for (int i = 1; i < argc; i++) {
        if ((pid = fork()) < 0) {
            ERR("fork");
        }
        if (pid == 0) {
            child(argv[i][0]);
            exit(EXIT_SUCCESS);
        }
    }
    if (ppid == getpid()) {
        parent();
    }

    while (wait(NULL) > 0) {
    }
    return EXIT_SUCCESS;
}
