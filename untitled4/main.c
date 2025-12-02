#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define GOHOME 100
#define ERR(source) \
(fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

volatile sig_atomic_t running = 1;
volatile sig_atomic_t sender_pid = 0;
volatile sig_atomic_t go_home = 0;


void sethandler(void (*f)(int), int sigNo) {
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}
void sethandler2(void (*f)(int,siginfo_t*,void*), int sigNo) {
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_flags = SA_SIGINFO;       // potrzebne, żeby mieć siginfo_t
    act.sa_sigaction = f;
    sigemptyset(&act.sa_mask);
    sigaction(SIGUSR1, &act, NULL);    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void sig_allarm(int sig) {
    running = 0;
    go_home = GOHOME;
}
void sig_usr1(int sig) {

}
void sig_usr (int signo,siginfo_t*info,void*cont) {
    sender_pid=info->si_pid;
}
void sig_term(int sig) {
running =0;
}

void usage(char *name) {
    fprintf(stderr, "USAGE: %s m  p\n", name);
    fprintf(stderr,
            "m - number of 1/1000 milliseconds between signals [1,999], "
            "i.e. one milisecond maximum\n");
    fprintf(stderr, "p - after p SIGUSR1 send one SIGUSER2  [1,999]\n");
    exit(EXIT_FAILURE);
}

void child_work(int k, int p, int sick) {
    // sethandler(sig_usr1, SIGUSR1);
    sethandler2(sig_usr,SIGUSR1);
    sethandler(sig_allarm, SIGALRM);
    sethandler(sig_term, SIGTERM);
    int count = 0;
    pid_t pid = getpid();
    srand(pid);
    int time = rand() % 50 + 150;
    struct timespec t = {0, time*1000000};
    fprintf(stderr, "child  %d starts day sick %d\n", pid, sick);

    sigset_t mask;
    sigfillset(&mask);
    sigdelset(&mask, SIGUSR1);
    sigdelset(&mask, SIGTERM);
    if (sick==1) {
        alarm(k);
    }
    while (sick == 0 && running==1) {
        sigsuspend(&mask);
        fprintf(stderr,"child %d %d cought at me\n",pid,sender_pid);
        if (rand() % 100 + 1 < p) {
            sick = 1;
            fprintf(stderr, "child  %d gets sick \n",pid);
            alarm(k);
        }
    }

    while (running) {
        nanosleep(&t, NULL);
        kill(0,SIGUSR1);
        count++;
        fprintf(stderr, "child  %d coughing %d\n", pid, count);
    }
    fprintf(stderr, "child  %d exit with %d\n", pid, count);
    exit(count+go_home);
}

void parent_work(int t) {
    sethandler(sig_allarm,SIGALRM);
    alarm(t);
    fprintf(stderr, "KG  %d set alarm %d\n", getpid(), t);
    sigset_t m1;
    sigemptyset(&m1);
    sigaddset(&m1, SIGUSR1);
    sigaddset(&m1, SIGTERM);
    sigprocmask(SIG_BLOCK, &m1, NULL);

    sigset_t mask;
    sigfillset(&mask);
    sigdelset(&mask, SIGALRM);
    sigsuspend(&mask);

    fprintf(stderr, "KG  %d ends sim\n", getpid());
    kill(0,SIGTERM);
}

void create_children(int n, int k, int p) {
    int sick = 1;
    for (int i = 0; i < n; i++) {
        switch (fork()) {
            case 0:
                // sethandler(sig_handler, SIGUSR1);
                // sethandler(sig_handler, SIGUSR2);
                child_work(k, p, sick);
                exit(EXIT_SUCCESS);
            case -1:
                perror("Fork:");
                exit(EXIT_FAILURE);
        }
        sick = 0;
    }
}


int main(int argc, char **argv) {
    if (argc != 5)
        usage(argv[0]);

    int t = atoi(argv[1]);
    int k = atoi(argv[2]);
    int n = atoi(argv[3]);
    int p = atoi(argv[4]);

    if (t <= 1 || t > 100 || k <= 1 || k > 100 || p <= 1 || p > 100 || n <= 1 || n > 30)
        usage(argv[0]);

    create_children(n, k, p);
    parent_work(t);
    int status;
    pid_t pid;
    int i=0;
    int count =0,whome=0,cwh=0;
    fprintf(stderr, "No. | Child ID | Status\n");
    while ((pid=wait(&status)) > 0) {
        i++;
        count = WEXITSTATUS(status);
        whome = 0;
        if (count > 100) {
            count -= 100;
            whome=1;
            cwh++;
        }
        fprintf(stderr, "%d | %d | coughed %d times and ",i, pid, count);
        if (whome==1) {
            fprintf(stderr, "went home\n");
        }
        else {
            fprintf(stderr, "stayed at KG\n");
        }
    }
    fprintf(stderr, "%d out of %d went home\n",cwh,i);
    exit(EXIT_SUCCESS);
}
