/**
 * =============================================================================
 *
 *       Filename:  exec.c
 *
 *    Description:  Collection of function to spawn and interact
 *                  with child processes
 *
 *        Version:  1.0
 *        Created:  08/21/2014 07:45:49 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Oliver Lorenz (ol), olli@olorenz.org
 *
 * =============================================================================
 */

#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "exec.h"

#define ROOT_UID		0

#define NUM_PIPES		3

#define PIPE_STDIN		0
#define PIPE_STDOUT		1
#define PIPE_STDERR		2

#define PIPE_RD_FD		0
#define PIPE_WR_FD		1

static int drop_privileges(const struct passwd *user)
{
	if (setgid(user->pw_gid))
		return 1;

	if (initgroups(user->pw_name, user->pw_gid))
		return 1;

	if (setuid(user->pw_uid))
		return 1;

	if (setuid(ROOT_UID) == 0) {
		errno = EPERM;
		return 1;
	}

	return 0;
}

static struct passwd *resolve_user(user_info_t user, enum user_info_type type)
{
	struct passwd *_user = NULL;

	errno = 0;
	switch (type) {
	case USERINFO_TYPE_UID:
		_user = getpwuid(*(user.ui_uid));
		break;
	case USERINFO_TYPE_NAME:
		_user = getpwnam(user.ui_name);
		break;
	case USERINFO_TYPE_NONE:
		/* fallthrough */
	default:
		errno = EINVAL;
		break;
	}

	if (!_user && !errno)
		errno = ENOENT;

	return _user;
}

int wait_for_child(struct process_info *proc, bool close_fds)
{
	pid_t child;

	do {
		child = waitpid(proc->pi_pid, &(proc)->pi_retval, 0);
	} while (child == (pid_t) - 1 && errno == EINTR);

	if (close_fds) {
		close(proc->pi_stdin);
		close(proc->pi_stdout);
		close(proc->pi_stderr);
		proc->pi_stdin  = -1;
		proc->pi_stdout = -1;
		proc->pi_stderr = -1;
	}

	return (child == (pid_t) - 1 ? -1 : 0);
}

void get_exit_details(int status, int *ret, bool *core, bool *signaled, bool *parent)
{
	(*ret)      = -1;
	(*core)     = false;
	(*signaled) = false;
	(*parent)   = false;

	if (status < 0) {
		if (status > -EXEC_PROCESS_ERROR_OFFSET) {
			/* status contains parent's errno */
			errno = -status;
			(*parent) = true;
		} else {
			status += EXEC_PROCESS_ERROR_OFFSET;
			errno = -status;
		}
		return;
	}

	if (unlikely(WIFSIGNALED(status))) {
		if (WCOREDUMP(status))
			(*core) = true;
		(*signaled) = true;
		(*ret) = WTERMSIG(status);
	} else if (WIFEXITED(status)) {
		(*ret) = WEXITSTATUS(status);
	}
}

int copy_exit_detail_str(int status, char **buffer)
{
	int ret;
	bool core;
	bool signaled;
	bool parent;

	get_exit_details(status, &ret, &core, &signaled, &parent);
	if (ret == -1) {
		if (parent)
			ret = asprintf(buffer, "An error has occurred in the "
			               "parent process: %s", strerror(errno));
		else
			ret = asprintf(buffer, "An error has occurred in the "
			               "child process: %s", strerror(errno));
	} else {
		if (signaled)
			ret = asprintf(buffer, "Process exited due to signal "
			               "%d (core %s dumped)", ret,
			               core ? "was" : "not");
		else
			ret = asprintf(buffer, "Process exited with status %d",
			               ret);
	}

	return (ret == -1);
}

ssize_t timed_read(int fd, void *buf, size_t size, unsigned int timeout)
{
	int ret;
	fd_set set;
	struct timeval wait_time;

	if (!timeout)
		return read(fd, buf, size);

	FD_ZERO(&set);
	FD_SET(fd, &set);

	wait_time.tv_sec  = timeout;
	wait_time.tv_usec = 0;

	ret = select(fd + 1, &set, NULL, NULL, &wait_time);
	if (ret == 0) {
		errno = ETIMEDOUT;
		return -1;
	} else if (ret == -1) {
		return -1;
	} else {
		ssize_t count;

		ret = fcntl(fd, F_GETFD, NULL);
		if (ret == -1)
			return -1;
		fcntl(fd, F_SETFD, ret | O_NONBLOCK);
		count = read(fd, buf, size);
		(void) fcntl(fd, F_SETFD, ret);

		return count;
	}
}

ssize_t timed_write(int fd, const void *buf, size_t size, unsigned int timeout)
{
	int ret;
	fd_set set;
	struct timeval wait_time;

	if (!timeout)
		return write(fd, buf, size);

	FD_ZERO(&set);
	FD_SET(fd, &set);

	wait_time.tv_sec  = timeout;
	wait_time.tv_usec = 0;

	ret = select(fd + 1, NULL, &set, NULL, &wait_time);
	if (ret == 0) {
		errno = ETIMEDOUT;
		return -1;
	} else if (ret == -1) {
		return -1;
	} else {
		ssize_t count;

		ret = fcntl(fd, F_GETFD, NULL);
		if (ret == -1)
			return -1;
		fcntl(fd, F_SETFD, ret | O_NONBLOCK);
		count = write(fd, buf, size);
		(void) fcntl(fd, F_SETFD, ret);

		return count;
	}
}

_sentinel int exec_process(struct process_info *proc_info, bool wait,
                           user_info_t user, enum user_info_type user_type,
                           const char *cmd, ...)
{
	int res;
	unsigned int i, num_args;
	char **args = NULL;
	va_list ap;

	num_args = 0;
	va_start(ap, cmd);
	while (va_arg(ap, char *))
		++num_args;
	va_end(ap);

	/* cmd + NULL */
	num_args += 2;

	if ((args = malloc(num_args * sizeof(*args))) == NULL) {
		res = -errno;
		goto exit;
	}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
	args[0] = (char *)cmd;
#pragma GCC diagnostic pop

	va_start(ap, cmd);
	for (i = 0; i < num_args - 2; ++i)
		args[i + 1] = va_arg(ap, char *);
	va_end(ap);

	args[num_args - 1] = NULL;

	res = exec_process_p(proc_info, wait, user, user_type, cmd, args);
exit:
	free(args);
	return res;
}

int exec_process_p(struct process_info *proc_info, bool wait,
                   user_info_t user, enum user_info_type user_type,
                   const char *cmd, char *const argv[])
{
	int pipes[NUM_PIPES][2];
	int self_pipe[2] = { -1, -1 };
	unsigned int i;
	int flags, child_error;
	pid_t pid;
	int res;

	for (i = 0; i < NUM_PIPES; ++i) {
		pipes[i][PIPE_RD_FD] = -1;
		pipes[i][PIPE_WR_FD] = -1;
	}

	if (wait) {
		/* Not needed if we're waiting */
		proc_info = NULL;
	}

	if (proc_info) {
		if (pipe(pipes[ PIPE_STDIN]) ||
		    pipe(pipes[PIPE_STDOUT]) || pipe(pipes[PIPE_STDERR])) {
			res = -errno;
			goto exit;
		}
	}

	if (pipe(self_pipe)) {
		res = -errno;
		goto exit;
	}

	flags = fcntl(self_pipe[PIPE_WR_FD], F_GETFD);
	if (flags == -1) {
		res = -errno;
		goto exit;
	}

	if (fcntl(self_pipe[PIPE_WR_FD], F_SETFD, flags | FD_CLOEXEC)) {
		res = -errno;
		goto exit;
	}

	pid = fork();
	if (pid == 0) {
		/* child */
		long maxfd;
		struct passwd *_user = NULL;

		if (proc_info) {
			dup2(pipes[ PIPE_STDIN][PIPE_RD_FD], STDIN_FILENO);
			dup2(pipes[PIPE_STDOUT][PIPE_WR_FD], STDOUT_FILENO);
			dup2(pipes[PIPE_STDERR][PIPE_WR_FD], STDERR_FILENO);

			close(pipes[PIPE_STDIN][PIPE_RD_FD]);
			close(pipes[PIPE_STDOUT][PIPE_WR_FD]);
			close(pipes[PIPE_STDERR][PIPE_WR_FD]);

			close(pipes[PIPE_STDIN][PIPE_WR_FD]);
			close(pipes[PIPE_STDOUT][PIPE_RD_FD]);
			close(pipes[PIPE_STDERR][PIPE_RD_FD]);
		}

		close(self_pipe[PIPE_RD_FD]);

		_user = resolve_user(user, user_type);
		if (!_user && errno != EINVAL /* USERINFO_TYPE_NONE */ )
			goto fail;

		if (_user && drop_privileges(_user))
			goto fail;

		maxfd = sysconf(_SC_OPEN_MAX);
		while (--maxfd > 3) {
			if (maxfd == self_pipe[PIPE_WR_FD])
				continue;
			close((int)maxfd);
		}

		execvp(cmd, argv);

fail:
		child_error = EXEC_PROCESS_ERROR_OFFSET + errno;
		write(self_pipe[PIPE_WR_FD], &child_error, sizeof(child_error));
		_exit(1);
	} else if (pid > 0) {
		/* parent */
		ssize_t count;

		if (proc_info) {
			close(pipes[PIPE_STDIN][PIPE_RD_FD]);
			close(pipes[PIPE_STDOUT][PIPE_WR_FD]);
			close(pipes[PIPE_STDERR][PIPE_WR_FD]);
		}

		close(self_pipe[PIPE_WR_FD]);

		while ((count = read(self_pipe[PIPE_RD_FD],
				     &child_error, sizeof(child_error))) == -1)
		{
			if (errno != EAGAIN && errno != EINTR) {
				break;
			}
		}

		close(self_pipe[PIPE_RD_FD]);

		if (count) {
			res = -child_error;
			goto exit;
		}

		if (wait) {
			int ret;
			pid_t child;

			do {
				child = waitpid(pid, &ret, 0);
			} while (child == (pid_t) - 1 && errno == EINTR);

			res = (child == (pid_t) - 1 ? errno : ret);
		} else {
			res = 0;
		}

		if (proc_info) {
			proc_info->pi_pid = pid;
			proc_info->pi_stdin  = pipes[ PIPE_STDIN][PIPE_WR_FD];
			proc_info->pi_stdout = pipes[PIPE_STDOUT][PIPE_RD_FD];
			proc_info->pi_stderr = pipes[PIPE_STDERR][PIPE_RD_FD];
		}
	} else {
		res = -errno;
		goto exit;
	}

	return res;

exit:
	(void) close(self_pipe[PIPE_RD_FD]);
	(void) close(self_pipe[PIPE_WR_FD]);
	for (i = 0; i < NUM_PIPES; ++i) {
		(void) close(pipes[i][PIPE_RD_FD]);
		(void) close(pipes[i][PIPE_WR_FD]);
	}
	if (proc_info)
		memset(proc_info, 0, sizeof(*proc_info));
	return res;
}
