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

#ifndef PROCEXEC_H
#define PROCEXEC_H

#include <stdbool.h>
#include <stdlib.h>
#include "compiler.h"


/**
 * struct process_info - process information
 * @pi_pid:		PID of the process
 * @pi_stdin:		file descriptor connected to stdin of process %pi_pid
 * @pi_stdout:		file descriptor connected to stdout of process %pi_pid
 * @pi_stderr:		file descriptor connected to stderr of process %pi_pid
 * @pi_retval:		holds the exit status once the process has exited
 *
 * This struct will be filled by sgExecProcess() to maintain
 * two-way communication with the child process once the function
 * returns. The file descriptors referring to stdin, stdout, and stderr
 * respectively of the child process have to be closed manually or by calling
 * sgWaitForChild() with @closeFds set to %true once they are of no use anymore
 */
struct process_info {
	pid_t pi_pid;
	int   pi_stdin;
	int   pi_stdout;
	int   pi_stderr;
	int   pi_retval;
};


/**
 * enum user_info_type - user information type information
 * @USERINFO_TYPE_NONE:		invalid
 * @USERINFO_TYPE_UID:		the provided information is a user's UID
 * @USERINFO_TYPE_NAME:		the provided information is a user's name
 */
enum user_info_type {
	USERINFO_TYPE_NONE,
	USERINFO_TYPE_UID,
	USERINFO_TYPE_NAME
};


/**
 * user_info_t - user identification
 * @ui_uid:			user UID
 * @ui_name:			user name
 */
typedef union {
	uid_t      *ui_uid;
	const char *ui_name;
} _transparent_union user_info_t;


#define EXEC_PROCESS_ERROR_OFFSET		255


/**
 * exec_process - execute a file
 * @proc:			storage space for process information
 * @wait:			if set, wait until @cmd terminates
 * @user:			if set, drop privileges before executing @cmd
 * @user_type:			if @user is non-null, indicates the type of
 *                              user information (uid or name)
 * @cmd:			the file to be executed
 * @...:			NULL-terminated list of arguments passed to @cmd
 *
 * exec_process() performs a fork() and depending on its parameters, behaves as
 * follows:     - if @wait is set to %true, @proc is ignored and the function
 *                returns only when @cmd has terminated
 *              - if @wait is set to %false, the function returns as soon as
 *                @cmd has been executed. Additionally, if @proc is non-null,
 *                it will be filled with the child's PID and file descriptors
 *                to its standard input, output, and standard error
 *              - if @user is non-null, the child drops its privileges after
 *                forking. Supported types for the value specified in @user
 *                are the user's UID or name. @user_type indicates which one
 *                is actually used.
 *
 * @return:			- On success, %0 is returned.
 *                              - If an error occurrs in the parent process
 *                                (before fork() was called), the return value
 *                                is in [ -1, -EXEC_PROCESS_ERROR_OFFSET ]
 *                              - If an error occurrs in the child process,
 *                                (after fork() was called), the return value
 *                                is in [ -EXEC_PROCESS_ERROR_OFFSET, ...]
 *                              In any case, the returned error code is
 *                              actually just the processes @errno.
 */
extern _sentinel int exec_process(struct process_info *proc, bool wait,
                                  user_info_t user,
                                  enum user_info_type user_type,
                                  const char *cmd, ...);


/**
 * exec_process - execute a file
 * @proc:			storage space for process information
 * @wait:			if set, wait until @cmd terminates
 * @user:			if set, drop privileges before executing @cmd
 * @user_type:			if @user is non-null, indicates the type of
 *                              user information (uid or name)
 * @cmd:			the file to be executed
 * @...:			NULL-terminated list of arguments passed to @cmd
 *
 * exec_process() performs a fork() and depending on its parameters, behaves as
 * follows:     - if @wait is set to %true, @proc is ignored and the function
 *                returns only when @cmd has terminated
 *              - if @wait is set to %false, the function returns as soon as
 *                @cmd has been executed. Additionally, if @proc is non-null,
 *                it will be filled with the child's PID and file descriptors
 *                to its standard input, output, and standard error
 *              - if @user is non-null, the child drops its privileges after
 *                forking. Supported types for the value specified in @user
 *                are the user's UID or name. @user_type indicates which one
 *                is actually used.
 *
 * @return:			- On success, %0 is returned.
 *                              - If an error occurrs in the parent process
 *                                (before fork() was called), the return value
 *                                is in [ -1, -EXEC_PROCESS_ERROR_OFFSET ]
 *                              - If an error occurrs in the child process,
 *                                (after fork() was called), the return value
 *                                is in [ -EXEC_PROCESS_ERROR_OFFSET, ...]
 *                              In any case, the returned error code is
 *                              actually just the processes @errno.
 */
extern int exec_process_p(struct process_info *proc, bool wait,
                          user_info_t user, enum user_info_type user_type,
                          const char *cmd, char *const argv[]);


/**
 * wait_for_child - wait for a child process to terminate
 * @proc:			process information
 * @close_fds:			if %true, open file descriptors to the child's
 *                              standard input, output and standard error are
 *                              closed upon process termination.
 * @return:			On sucess, %0 is returned, otherwise the
 *                              function returns %-1 and @errno is set
 *                              according to waitpid(2).
 */
extern int wait_for_child(struct process_info *proc, bool close_fds);


/**
 * timed_read - read from a file descriptor
 * @fd:				file descriptor to read from
 * @buf:			buffer for the returned data
 * @size:			size of the buffer pointed to by @buf
 * @timeout:			time to wait for data
 *
 * timed_read() attempts to read up to @size bytes from file descriptor @fd
 * into the buffer starting at @buf. If @timeout was set to %0, this
 * function behaves exactly like read(2). Otherwise, select(2) will be used
 * to wait up to @timeout seconds until returning an error.
 *
 * @return:			On success, the number of bytes read is
 *                              returned (zero indicates end of file).
 *                              On error, -1 is returned, and errno is set
 *                              according to read(2) and select(2).
 */
extern ssize_t timed_read(int fd, void *buf, size_t size, unsigned int timeout);


/**
 * timed_write - write to a file descriptor
 * @fd:				file descriptor to write to
 * @buf:			buffer containing the data to be written
 * @size:			size of the buffer pointed to by @buf
 * @timeout:			time to wait for @fd to become ready
 *
 * timed_write() attempts to write up to @size bytes from the buffer starting
 * at @buf to the file descriptor @fd. If @timeout was set to %0, this
 * function behaves exactly like write(2). Otherwise, select(2) will be used
 * to wait up to @timeout seconds until returning an error.
 *
 * @return:			On success, the number of bytes written is
 *                              returned (zero indicates end of file).
 *                              On error, -1 is returned, and errno is set
 *                              according to write(2) and select(2).
 */
extern ssize_t timed_write(int fd, const void *buf,
                           size_t size, unsigned int timeout);


/**
 * get_exit_details - get information about a process exit value
 * @status:			exit status
 * @ret:			the real return value
 * @core:			indicator if core was dumped
 * @signaled:			indicator if process was killed by a signal
 * @parent:			indicator if error has occurred in the parent
 *                              process (only valid if @status < 0)
 *
 * get_exit_details() takes the process exit value @status (typically
 * obtained by waitpid() and co) and sets the parameters as follows:
 *      - if the process exited normally (exit, _exit, or return from main),
 *        @ret will be set to the exit value
 *      - if the process was killed by a signal, @signaled will be set to
 *        %true and @ret will be set to the signal that caused the abortion
 *      - if the process was killed by a signal, @core will indicate if a
 *        core dump has been created when the process died (and as before,
 *        @ret will hold the signal number)
 */
extern void get_exit_details(int status, int *ret, bool *core,
                             bool *signaled, bool *parent);


/**
 * copy_exit_detail_str - obtain a printable description for an exit status
 * @status:			exit status
 * @buffer:			pointer to a to-be-allocated buffer
 *
 * copy_exit_detail_str() takes the process exit value @status (typically
 * obtained by waitpid() and co) and fills the buffer @buffer with a
 * printable description. Information includes:
 *      - the return code
 *      - if killed by a signal, the signal number
 *      - if killed by a signal, whether the core was dumped
 *
 * @return:			0 for success, 1 otherwise.
 *                              errno will be set according to asprintf(3).
 */
extern int copy_exit_detail_str(int status, char **buffer);

#endif
