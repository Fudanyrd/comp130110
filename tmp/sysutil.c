#include "sysutil.h"

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>

static int Fork()
{
    int pid = fork();
    if (pid < 0) {
        perror("fork failure");
        exit(1);
    }
    return pid;
}

static void Pipe(int pipfd[2])
{
    if (pipe(pipfd) != 0) {
        perror("cannot create pipe");
        exit(1);
    }
}

static void Dup2(int old, int nw)
{
    if (dup2(old, nw) != nw) {
        perror("error dup2");
        exit(1);
    }
}

static void Exit(int code)
{
    exit(code);
}

static int Open(const char *fn, int flag)
{
    int fd;
    if ((fd = open(fn, flag)) < 0) {
        perror("open failure");
        exit(1);
    }
    return fd;
}

// allocate space for tokens(most less than 128 bytes)
static void *Malloc(unsigned sz)
{
    void *ret = malloc(sz);
    if (ret == NULL) {
        perror("malloc failure");
        exit(1);
    }
    return ret;
}

// free memory occupied by Malloc
static void Free(void *b)
{
    if (b != NULL) {
        free(b);
    }
}

// use sep to join several strings.
static char *strjoin(const char *sep, const char *s1, const char *s2)
{
    size_t lsep = strlen(sep);
    size_t ls1 = strlen(s1);
    size_t ls2 = strlen(s2);
    char *ret = Malloc(1U + lsep + ls1 + ls2);
    // copy s1.
    memcpy(ret, s1, ls1);
    memcpy(ret + ls1, sep, lsep);
    memcpy(ret + ls1 + lsep, s2, ls2);
    ret[lsep + ls1 + ls2] = '\0';
    return ret;
}

static void Execve(char *argc, char **argv)
{
    // look into current working dir
    execve(argc, argv, NULL);
    // then search in root dir
    char *rt = strjoin("/", "/usr/bin", argc);
    execve(rt, argv, NULL);
    // failed, abort
    Free(rt);
    exit(1);
}

// execute a single job. Does not return on success.
static void job_exec_single(struct job_t *job)
{
    // deal with IO redirection here
    if (job->stdin2 != NULL) {
        int fd = Open(job->stdin2, O_RDONLY);
        Dup2(fd, 0);
    }
    if (job->stdout2 != NULL) {
        int fd = Open(job->stdout2, O_CREAT | O_RDWR);
        Dup2(fd, 1);
    }
    if (job->stderr2 != NULL) {
        int fd = Open(job->stderr2, O_CREAT | O_RDWR);
        Dup2(fd, 2);
    }
    // no need to consider pipe
    Execve(job->exe, job->argv);
}

// no return version
void job_exec_noret(struct job_t *job)
{
    if (job->nxt == NULL) {
        job_exec_single(job);
        Exit(1);
    }

    int pd[2];
    if (job->pip) {
        Pipe(pd);
    }

    int chd = Fork();
    if (chd == 0) {
        // child
        if (job->pip) {
            Dup2(pd[0], 0);
            close(pd[0]);
            close(pd[1]);
        }
        job_exec_noret(job->nxt);
        Exit(1);
    } else {
        if (job->pip) {
            Dup2(pd[1], 1);
            close(pd[0]);
            close(pd[1]);
        }
        job_exec_single(job);
        Exit(waitpid(chd, NULL, 0));
    }
}

int job_exec(struct job_t *job)
{
    if (job == NULL) {
        return 0;
    }

    // tests:
    // echo foo bar
    // echo foo bar | cat
    // usr/bin/echo foo bar | /usr/bin/cat | /usr/bin/wc
    if (job->exe == NULL || job->argv == NULL) {
        return -1;
    }
    int chd = Fork();
    if (chd == 0) {
        job_exec_noret(job);
    }

    return waitpid(chd, NULL, 0);
}

static int stricmp(const char *src, const char *tar, int i, int k)
{
    for (int it = 0; it < k - i; it++) {
        if (tar[it] == '\0') {
            return src[it];
        }
        if (tar[it] != src[it + i]) {
            return src[it + i] - tar[it];
        }
    }

    return 0;
}

static void stricpy(char *dst, const char *src, int i, int k)
{
    int it;
    for (it = 0; it < k - i; it++) {
        dst[it] = src[it + i];
    }
    dst[it] = '\0';
}

int System(const char *cmd)
{
#define zero_out(dat) memset(dat, 0, sizeof(dat))
    static struct job_t jobs[MAXJOB];
    zero_out(jobs);

    int i = 0, k;
    int cnt = 0; // job count;
    int narg = 0; // # arguments
#define skip_whites                                            \
    for (k = i; cmd[k] != 0; k++) {                            \
        if (cmd[k] != ' ' && cmd[k] != '\t' && cmd[k] != '\r') \
            break;                                             \
    }
#define tok_end                                                \
    for (k = i; cmd[k] != 0; k++) {                            \
        if (cmd[k] == ' ' || cmd[k] == '\t' || cmd[k] == '\r') \
            break;                                             \
    }

    // start parsing
    while (cmd[i] != 0) {
        skip_whites;
        i = k;
        tok_end;
        // find a token here, start at i, end at j
        if (i < k) {
            // check if is an end token.
            if (stricmp(cmd, ";", i, k) == 0 || stricmp(cmd, "|", i, k) == 0 ||
                stricmp(cmd, "&", i, k) == 0) {
                // check pipe
                jobs[cnt].pip = cmd[i] == '|' ? 1 : 0;
                jobs[cnt].argv[narg] = NULL;
                cnt++;
                narg = 0;
                if (cnt > 0)
                    jobs[cnt - 1].nxt = &jobs[cnt];
                // end of parsing a job.
            } else {
                // a normal token.
                if (narg == 0) {
                    char *dst = (char *)Malloc(k - i + 1);
                    stricpy(dst, cmd, i, k);
                    jobs[cnt].argv[narg] = dst;
                    jobs[cnt].exe = dst;
                } else {
                    if (stricmp(cmd, ">", i, k) == 0 ||
                        stricmp(cmd, "<", i, k) == 0 ||
                        stricmp(cmd, "2>", i, k) == 0) {
                        const char lead = cmd[i];
                        i = k;
                        skip_whites;
                        i = k;
                        tok_end;
                        switch (lead) {
                        case '>': { // stdout
                            jobs[cnt].stdout2 = Malloc(k - i + 1);
                            stricpy(jobs[cnt].stdout2, cmd, i, k);
                            break;
                        }
                        case '<': { // stdin
                            jobs[cnt].stdin2 = Malloc(k - i + 1);
                            stricpy(jobs[cnt].stdin2, cmd, i, k);
                            break;
                        }
                        case '2': { // stderr
                            jobs[cnt].stderr2 = Malloc(k - i + 1);
                            stricpy(jobs[cnt].stderr2, cmd, i, k);
                            break;
                        }
                        }

                    } else {
                        char *dst = Malloc(k - i + 1);
                        stricpy(dst, cmd, i, k);
                        jobs[cnt].argv[narg] = dst;
                    }
                }
                narg++;
            }
        }
        i = k;
    }

    int ret = job_exec((struct job_t *)jobs);

    // destroy all jobs
    for (int i = 0; i < cnt; i++) {
        // notice that exe and argv[0] is shared.
        // so only one need to be freed.
        for (int k = 0; jobs[i].argv[k] != NULL; k++) {
            Free(jobs[i].argv[k]);
        }
        Free(jobs[i].stdin2);
        Free(jobs[i].stdout2);
        Free(jobs[i].stderr2);
    }

    return ret;
}

#if 0
// this is a draft job_exec impl(and an incorrect one)
int job_exec(struct job_t *job)
{
    if (job == NULL) {
        return 0;
    }

    // tests:
    // echo foo bar
    // echo foo bar | cat
    // usr/bin/echo foo bar | /usr/bin/cat | /usr/bin/wc
    if (job->exe == NULL || job->argv == NULL) {
        return -1;
    }

    struct job_t *it = job;
    int ret;

    // pip descriptor
    int pipfd[2];
    int pip = it->pip;
    if (pip) {
        Pipe(pipfd);
    }

    if (it->nxt == NULL) {
        // simply exec and exit.
        int pid = Fork();
        if (pid == 0) {
            execve(it->exe, it->argv, NULL);
        } else {
            ret = waitpid(pid, NULL, 0);
        }
        return ret;
    }

    int pid = Fork();

    if (pid == 0) {
        // child
        if (pip) {
            // close write port of the pipe.
            // change stdin to pipfd[0].
            Dup2(pipfd[0], 0);
            close(pipfd[1]);
            close(pipfd[1]);
        }

        if (it->nxt != NULL) {
            exit(job_exec(it->nxt));
        }
    } else {
        int pid2 = Fork();
        if (pid2 == 0) {
            if (pip) {
                // change stdout to write port
                Dup2(pipfd[1], 1);
                close(pipfd[0]);
                close(pipfd[1]);
            }
            execve(it->exe, it->argv, NULL);
            // failed to execute
            exit(1);
        } else {
            close(pipfd[0]);
            close(pipfd[1]);
            ret = waitpid(pid, NULL, 0);
        }
        ret = waitpid(pid2, NULL, 0);
    }

    return ret;
}
#endif
