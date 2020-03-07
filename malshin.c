#define _GNU_SOURCE
#include <sched.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <fcntl.h>

#define HOSTNAME "malshin"

#define OLD_ROOT "old_root"
#define NEW_ROOT "new_root"

#define UID_MAP_STR  "0 0 4294967295"

#define pivot_root(new_root, put_old) syscall(SYS_pivot_root, new_root, put_old)

int init_env()
{
	// Change host name
	if (sethostname(HOSTNAME, strlen(HOSTNAME)) == -1)
		goto init_err;

	/*
	 * Changes the root directory to a dedicated directory, also unmounts all other directories.
	 * This will make impossible to go out of the container - except by calling exit.
	 */
	// Mount everything as private (in this mount namespace)
	if (mount("none", "/", NULL, MS_PRIVATE | MS_REC, NULL) == -1)
		goto init_err;

	// Make a sperate mount
	if (mount(NEW_ROOT, NEW_ROOT, NULL, MS_BIND | MS_PRIVATE | MS_REC, NULL) == -1)
		goto init_err;

	// Go to the new mount
	if (chdir(NEW_ROOT) == -1)
		goto init_err;

	// Make a directory for pivot_root
	if (mkdir(OLD_ROOT, 0775) == -1)
		goto init_err;

	// Move / to old_root and make this directory the new root
	if (pivot_root(".", OLD_ROOT) == -1)
		goto init_err;

	// Change directory to the new root
	if (chdir("/") == -1)
		goto init_err;

	// umount the old root
	if (umount2(OLD_ROOT, MNT_DETACH) == -1)
		goto init_err;

	// Remove the old root directory
	if (rmdir(OLD_ROOT) == -1)
		goto init_err;

	/*
	 * This part mounts the procfs on /proc inside the new root.
	 * The new /proc will only show processes in the new pid_namespace.
	 */
	// Make a directory for /proc
	if (mkdir("/proc", 0755) == -1 && errno != EEXIST)
		goto init_err;

	// Mount the /proc directory
	if (mount("proc", "/proc", "proc", 0, NULL) == -1)
		goto init_err;

	return 0;

init_err:
	printf("init_env errno: (%d) %s\n", errno, strerror(errno));
	return -1;
}

void init()
{
	char* const args[] = {"/bin/sh", NULL};

	printf("pid: %d\n", getpid());

	if (init_env() == -1)
		exit(3);

	if (fork() == 0) {
		printf("fork successful child pid: %d\n", getpid());

		if (execvp(args[0], &args[0]) == -1)
			printf("execvp failed: %d -> %s\n", errno, strerror(errno));

		exit(1);
	}

	while (1)
	{
		if (wait(NULL) != -1)
			continue;

		if (errno == 10)
			printf("maslhin exiting...(%d) %s\n", errno, strerror(errno));
		else
			printf("malshin init process exited - wait failed with errno: (%d) %s\n", errno, strerror(errno));

		exit(1);
	}

	printf("malshin init process exited loop when it shouldn't\n");
	exit(2);
}

int main()
{
	// Isloate file descriptors and filesystem attributes - like root directory etc.
	if (unshare(CLONE_FILES | CLONE_FS) == -1)
		goto unshare_err;

	// Isolate attributes that show up in uname
	if (unshare(CLONE_NEWUTS) == -1)
		goto unshare_err;

	/*
	// Unshare user namespace
	if (unshare(CLONE_NEWUSER) == -1)
		goto unshare_err;
	*/

	// Unshare cgroup
	if (unshare(CLONE_NEWCGROUP) == -1)
		goto unshare_err;

	// Isolate IPC
	if (unshare(CLONE_NEWIPC) == -1)
		goto unshare_err;

	// Unshare pid namespace
	if (unshare(CLONE_NEWPID) == -1)
		goto unshare_err;

	// Unshare mount namespace
	if (unshare(CLONE_NEWNS) == -1)
		goto unshare_err;

	// Unshare network namespace
	if (unshare(CLONE_NEWNET) == -1)
		goto unshare_err;

	if (fork() == 0)
		init();

	wait(NULL);
	return 0;

unshare_err:
	printf("errno: (%d) %s\n", errno, strerror(errno));
	return -1;
}
