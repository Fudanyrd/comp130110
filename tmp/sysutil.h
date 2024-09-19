#ifndef __SYSUTIL_H_
#define __SYSUTIL_H_

/**
 * This is an illustration-purpose package to show the 
 * usage of common syscalls like execve, read, write,
 * pipe, dup2, etc. Meanwhile, the System function 
 * makes it easier to create a shell(like shell.c).
 */

/** Maximum length of input string */
#define MAXLINE 8192
/** Maximum of jobs to run simultaneously */
#define MAXJOB 8
#define MAXARGV 32

struct job_t {
	char *exe;
	char *argv[MAXARGV];
	char *stdin2;  // redirect stdin to
	char *stdout2;  // redirect stdin to
	char *stderr2;  // redirect stdin to
	int pip;       // create pipe?
	struct job_t *nxt;
};

/**
 * Execute a list of jobs.
 * @return the exit code of job.
 */
int job_exec(struct job_t *job);
void job_exec_noret(struct job_t *job);

/** Same as system function provided in stdlib.
 * Support IO redirection("<", ">" and "2>") and pipe("|").
 * @return the exit code of the command.
 */
int System(const char *cmd);

/**
 * Load and execute a program.
 */
void loader(char *argc, char **argv);

#endif // __SYSUTIL_H_
