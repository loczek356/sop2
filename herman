#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define ERR(source) \
    (kill(0, SIGKILL), perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

#define FILE_MAX_SIZE 512

volatile sig_atomic_t sig_usr1 = 0;
volatile sig_atomic_t sig_usr2 = 0;
volatile sig_atomic_t sig_int = 0;

void usr1_handler(int sig) { sig_usr1 = 1; }
void usr2_handler(int sig) { sig_usr2 = 1; }
void int_handler(int sig) { sig_int = 1; }

ssize_t bulk_read(int fd, char* buf, size_t count)
{
    ssize_t c;
    ssize_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(read(fd, buf, count));
        if (c < 0)
            return c;
        if (c == 0)
            return len;  // EOF
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

void usage(int argc, char* argv[])
{
    printf("%s p h\n", argv[0]);
    printf("\tp - path to directory describing the structure of the Austro-Hungarian office in Prague.\n");
    printf("\th - Name of the highest administrator.\n");
    exit(EXIT_FAILURE);
}

void sethandler(void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void work(char *name, sigset_t* oldmask, int is_root) {
    pid_t pid = getpid();
    printf("My name is %s and my pid is %d\n", name, pid);

    int fd = open(name, O_RDONLY);
    if (fd == -1) ERR("open");
    char buf[FILE_MAX_SIZE + 1] = {0};
    if (bulk_read(fd, buf, FILE_MAX_SIZE) < 0)
        ERR("bulk_read");

    close(fd);

    char* child_name = strtok(buf, "\n");

    pid_t children[2] = {0};
    int children_count = 0;

    while (child_name) {
        if (strcmp(child_name, "-") == 0)
        {
            child_name = strtok(nullptr, "\n");
            continue;
        }

        printf("%s inspecting %s\n", name, child_name);

        pid_t child = fork();
        if (child == -1) ERR("fork");
        if (child == 0) { // child process
            work(child_name, oldmask, 0);
            exit(EXIT_SUCCESS);
        }

        children[children_count++] = child;

        child_name = strtok(nullptr, "\n");
    }

    printf("%s has inspected all subordinates\n", name);

    srand(getpid());
    while (sigsuspend(oldmask))
    {
        if (sig_int)
        {
            printf("%s ending the day\n", name);
            if (children[0] > 0)
                kill(children[0], SIGINT);
            if (children[1] > 0)
                kill(children[1], SIGINT);

            break;
        }
        if (sig_usr2)
        {
            sig_usr2 = 0;
            switch (rand() % 3)
            {
                case 0:
                case 1:
                    if (is_root)
                    {
                        printf("%s received a document. Ignoring.\n", name);
                    }
                    else
                    {
                        printf("%s received a document. Passing on to the superintendent \n", name);
                        kill(getppid(), SIGUSR2);
                    }
                    break;
                case 2:
                    printf("%s received a document. Sending to the archive.\n", name);
                    break;
            }
        }
    }

    while (wait(nullptr) > 0) {}

    printf("%s leaving the office.\n", name);
}

int main(int argc, char* argv[]) {
    if (argc != 3)
    {
        usage(argc, argv);
    }

    if (chdir(argv[1]))
        ERR("chdir");

    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);
    sethandler(usr1_handler, SIGUSR1);
    sethandler(usr2_handler, SIGUSR2);
    sethandler(int_handler, SIGINT);

    printf("Waiting for SIGUSR1\n");

    while (!sig_usr1 && sigsuspend(&oldmask)){}

    printf("Got SIGUSR1\n");

    work(argv[2], &oldmask, 1);
}
