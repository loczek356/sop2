#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#define ERR(source) \
(fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

volatile sig_atomic_t running = 0;
volatile sig_atomic_t init = 1;


void sethandler(void (*f)(int), int sigNo) {
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void sig_usr1(int sig) {
    running = 1;
}

void sig_usr2(int sig) {
    running = 0;
}

void sig_init(int sig) {
    init = 0;
    running = 0;
}

void usage(char *name) {
    fprintf(stderr, "USAGE: %s m  p\n", name);
    fprintf(stderr,
            "m - number of 1/1000 milliseconds between signals [1,999], "
            "i.e. one milisecond maximum\n");
    fprintf(stderr, "p - after p SIGUSR1 send one SIGUSER2  [1,999]\n");
    exit(EXIT_FAILURE);
}

void child_work() {
    sethandler(sig_usr1, SIGUSR1);
    sethandler(sig_usr2, SIGUSR2);
    sethandler(sig_init, SIGINT);

    pid_t pid = getpid();
    int count = 0;
    srand(pid);
    int time = rand() % 100 + 100;
    struct timespec t = {1, time * 1000000};

    sigset_t mask;
    sigfillset(&mask);
    sigdelset(&mask, SIGUSR1);
    sigdelset(&mask, SIGINT);

    while (init) {
        sigsuspend(&mask);
        while (running) {
            nanosleep(&t, NULL);
            count++;
            fprintf(stderr, "{%d}: {%d}\n", pid, count);
        }
    }
    char name[32];
    sprintf(name, "%d.txt", pid);

    int out;
    if ((out = open(name, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0777)) < 0)
        ERR("open");

    char content[32];
    int len = sprintf(content, "%d", count);
    if (write(out, content, len) < 0) {
        ERR("write");
    }
    exit(EXIT_SUCCESS);
}

void parent_work(pid_t *kids, int N) {
    fprintf(stderr, "par pid{%d}\n", getpid());
    int i = 0;
    sigset_t mask;
    sigfillset(&mask);
    sigdelset(&mask, SIGUSR1);
    sigdelset(&mask, SIGINT);

    sethandler(sig_usr1, SIGUSR1);
    sethandler(sig_init, SIGINT);

    while (init) {
        kill(kids[i], SIGUSR1);
        sigsuspend(&mask);
        kill(kids[i], SIGUSR2);
        i++;
        if (i == N) {
            i = 0;
        }
    }
    kill(0, SIGINT);
}

void create_children(int n, pid_t *pids[]) {
    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        switch (pid) {
            case 0:
                pid_t cpid = getpid();
                fprintf(stderr, "my pid %d and my num %d\n", cpid, i);
                child_work();
                exit(EXIT_SUCCESS);
            case -1:
                perror("Fork:");
                exit(EXIT_FAILURE);
            default:
                (*pids)[i] = pid;
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 2)
        usage(argv[0]);

    int N = atoi(argv[1]);

    if (N < 1)
        usage(argv[0]);

    sethandler(sig_usr1, SIGUSR1);

    pid_t *kids = malloc(sizeof(pid_t) * N);

    create_children(N, &kids);
    parent_work(kids, N);


    while (wait(NULL) > 0) {
    }

    free(kids);
    exit(EXIT_SUCCESS);
}
