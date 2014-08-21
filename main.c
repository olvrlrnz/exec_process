/*
 * =============================================================================
 *
 *       Filename:  exec.c
 *
 *    Description:  Basic tests for exec_process
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
#include <signal.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include "exec.h"

#define BUFFER_SIZE		4096U

typedef int (*testfunc_t)(void);

struct testcase {
	testfunc_t tc_testfunc;
	int        tc_expected;
	bool       tc_prettyprint;
};

static int t0(void)
{
	return exec_process(NULL, true, NULL, USERINFO_TYPE_NONE,
	                    "/tmp/ret0.sh", NULL);
}

static int t1(void)
{
	return exec_process(NULL, true, NULL, USERINFO_TYPE_NONE,
	                    "/tmp/ret14.sh", NULL);
}

static int t2(void)
{
	return exec_process(NULL, true, NULL, USERINFO_TYPE_NONE,
	                    "/noent", NULL);
}

static int t3(void)
{
	return exec_process(NULL, true, NULL, USERINFO_TYPE_NONE,
	                    "/tmp/sigPipe.sh", NULL);
}

static int t4(void)
{
	return exec_process(NULL, true, NULL, USERINFO_TYPE_NONE,
	                    "/tmp/sigAbrt.sh", NULL);
}

static int t5(void)
{
	uid_t me = geteuid();

	return exec_process(NULL, true, &me, USERINFO_TYPE_UID,
	                    "/noent", NULL);
}

static int t6(void)
{
	const char *user = "root";

	return exec_process(NULL, true, user, USERINFO_TYPE_NAME,
	                    "/noent", NULL);
}

static int t7(void)
{
	int ret;
	struct process_info proc;

	ret = exec_process(&proc, false, NULL, USERINFO_TYPE_NONE,
	                   "/tmp/ret0.sh", NULL);
	if (ret)
		return ret;

	wait_for_child(&proc, true);
	return proc.pi_retval;
}

static int t8(void)
{
	int ret;
	struct process_info proc;

	ret = exec_process(&proc, false, NULL, USERINFO_TYPE_NONE,
	                   "/tmp/ret14.sh", NULL);
	if (ret)
		return ret;

	wait_for_child(&proc, true);
	return proc.pi_retval;
}

static int t9(void)
{
	int ret;
	struct process_info proc;

	ret = exec_process(&proc, false, NULL, USERINFO_TYPE_NONE,
	                   "/noent", NULL);
	if (ret)
		return ret;

	wait_for_child(&proc, true);
	return proc.pi_retval;
}

static int t10(void)
{
	int ret;
	struct process_info proc;

	ret = exec_process(&proc, false, NULL, USERINFO_TYPE_NONE,
	                   "/tmp/sigPipe.sh", NULL);
	if (ret)
		return ret;

	wait_for_child(&proc, true);
	return proc.pi_retval;
}

static int t11(void)
{
	int ret;
	struct process_info proc;

	ret = exec_process(&proc, false, NULL, USERINFO_TYPE_NONE,
	                   "/tmp/sigAbrt.sh", NULL);
	if (ret)
		return ret;

	wait_for_child(&proc, true);
	return proc.pi_retval;
}

static int t12(void)
{
	int ret;
	uid_t me = geteuid();
	struct process_info proc;

	ret = exec_process(&proc, false, &me, USERINFO_TYPE_UID,
	                   "/noent", NULL);
	if (ret)
		return ret;

	wait_for_child(&proc, true);
	return proc.pi_retval;
}

static int t13(void)
{
	int ret;
	const char *user = "root";
	struct process_info proc;

	ret = exec_process(&proc, false, user, USERINFO_TYPE_NAME,
	                   "/noent", NULL);
	if (ret)
		return ret;

	wait_for_child(&proc, true);
	return proc.pi_retval;
}

static int t14(void)
{
	int ret;
	char buffer[BUFFER_SIZE];
	struct process_info proc;

	ret = exec_process(&proc, false, NULL, USERINFO_TYPE_NONE,
	                   "/bin/ls", "-la", "/noent", NULL);
	if (ret)
		return ret;

	ret = (int) timed_read(proc.pi_stdout, buffer, sizeof(buffer), 2);
	if (ret == -1)
		return ret;

	if (ret > 0) {
		/* Got some bytes - should not happen */
		goto done;
	}

	ret = (int) timed_read(proc.pi_stderr, buffer, sizeof(buffer), 2);
	if (ret == -1)
		return ret;

	if (ret > 0) {
		/* Got some bytes - should hit because of ls failing */
		goto done;
	}

done:
	wait_for_child(&proc, true);
	return proc.pi_retval;
}

static int t15(void)
{
	int ret;
	char buffer[BUFFER_SIZE];
	struct process_info proc;

	ret = exec_process(&proc, false, NULL, USERINFO_TYPE_NONE,
	                   "/usr/bin/bc", "-q", NULL);
	if (ret)
		return ret;

	ret = snprintf(buffer, sizeof(buffer), "2 + 2\n");
	ret = (int) timed_write(proc.pi_stdin, buffer, (size_t) ret, 1);
	if (ret == -1)
		return ret;

	ret = (int) timed_read(proc.pi_stderr, buffer, sizeof(buffer), 2);
	if (ret == -1 && errno != ETIMEDOUT)
		return ret;

	if (ret > 0) {
		/* Got some bytes - should not happen */
		goto done;
	}

	ret = (int) timed_read(proc.pi_stdout, buffer, sizeof(buffer), 2);
	if (ret == -1 && errno != ETIMEDOUT)
		return ret;

	if (ret > 0) {
		/* Got some bytes - should print '4' */
		goto done;
	}

done:
	kill(proc.pi_pid, SIGTERM);
	wait_for_child(&proc, true);
	return proc.pi_retval;
}

const struct testcase testcases[] = {
	/* wait tests */
	{ t0,	    0,	true },
	{ t1,	 3584,	true },
	{ t2,	- 257,	true },
	{ t3,	   13,	true },
	{ t4,	  134,	true },
	{ t5,	- 256,	true },
	{ t6,	- 256,	true },

	/* no-wait tests */
	{ t7,	    0,	true },
	{ t8,	 3584,	true },
	{ t9,	- 257,	true },
	{ t10,	   13,	true },
	{ t11,	  134,	true },
	{ t12,	- 256,	true },
	{ t13,	- 256,	true },

	/* random tests */
	{ t14,	  512,	true },
	{ t15,	   15,	true }
};

int main(void)
{
	const struct testcase *test;
	int i, failed, numTests;

	failed = 0;
	numTests = ARRAY_SIZE(testcases);
	for (i = 0; i < numTests; ++i) {
		int ret;

		test = &testcases[i];
		printf("Running test case %d ...\n", i + 1);
		ret = test->tc_testfunc();
		if (ret == test->tc_expected) {
			if (test->tc_prettyprint) {
				char *exitString = NULL;
				(void)copy_exit_detail_str(ret, &exitString);
				if (exitString) {
					printf("%s", exitString);
					free(exitString);
				}
			}
		} else {
			++failed;
		}
		printf("\n\n");
	}

	return failed;
}
