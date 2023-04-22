#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#define RUN_IN_BACKGROUND '&'
#define PIPE '|'
#define REDIRECT '<'
#define TRUE 1
#define FALSE 0
#define SUCCESS 0
#define NOT_FOUND -1

int handle_background(int count, char **arglist);
int handle_pipe(int count, char **arglist);
int handle_redirect(int count, char **arglist);
int execute_pipe(int count, char **arglist, int pipe_index);
int execute_redirect(int count, char **arglist);
void execute_command(int count, char **arglist, int is_background);
void execute_pipe_child(char **arglist, int *fd, int is_out);
void execute_redirect_child(int count, char **arglist);

// Compile with: gcc -O3 -D_POSIX_C_SOURCE=200809 -Wall -std=c11 shell.c myshell.c

int prepare(void)
{
	return SUCCESS;
}

int process_arglist(int count, char **arglist)
{

	int pipe_return = handle_pipe(count, arglist);
	if (pipe_return != NOT_FOUND)
	{
		return pipe_return;
	}

	int redirect_return = handle_redirect(count, arglist);
	if (redirect_return != NOT_FOUND)
	{
		return redirect_return;
	}

	if (!handle_background(count, arglist))
	{
		execute_command(count, arglist, FALSE);
	}

	return 1;
}

int finalize(void)
{
	return SUCCESS;
}

int handle_background(int count, char **arglist)
{
	if (arglist[count - 1][0] != RUN_IN_BACKGROUND)
	{
		return FALSE;
	}

	arglist[count - 1] = NULL;
	// printf("Background\n");
	execute_command(count - 1, arglist, TRUE);
	return TRUE;
}

int handle_pipe(int count, char **arglist)
{
	// First and last word cannot be pipes.
	for (int i = 1; i < count - 1; i++)
	{
		// We assume at most 1 pipe, so we return after execution.
		if (arglist[i][0] == PIPE)
		{
			return execute_pipe(count, arglist, i);
		}
	}
	return NOT_FOUND;
}

int handle_redirect(int count, char **arglist)
{
	// If there is a redirect, it is guaranteed to be the second-to-last word.
	if (count > 2 && arglist[count - 2][0] == REDIRECT)
	{
		return execute_redirect(count, arglist);
	}
	return NOT_FOUND;
}

int execute_pipe(int count, char **arglist, int pipe_index)
{
	arglist[pipe_index] = NULL;
	char **args_out = arglist;
	char **args_in = arglist + pipe_index + 1;

	int fd[2];
	pipe(fd);

	pid_t pid_child_out = fork();
	if (pid_child_out == NOT_FOUND)
	{
		perror(strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (pid_child_out == 0)
	{
		execute_pipe_child(args_out, fd, TRUE);
	}

	pid_t pid_child_in = fork();
	if (pid_child_in == NOT_FOUND)
	{
		perror(strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (pid_child_in == 0)
	{
		execute_pipe_child(args_in, fd, FALSE);
	}

	// Parent closes both ends of pipe and waits for both children
	close(fd[0]);
	close(fd[1]);
	waitpid(pid_child_out, NULL, 0);
	waitpid(pid_child_in, NULL, 0);

	return 1;
}

int execute_redirect(int count, char **arglist)
{
	pid_t pid = fork();
	if (pid == NOT_FOUND)
	{
		perror(strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (pid == 0)
	{
		execute_redirect_child(count, arglist);
	}
	else
	{
		int status;
		wait(&status);
		if (WIFEXITED(status))
		{
			// printf("Child finished\n");
		}
	}

	return 1;
}

void execute_command(int count, char **arglist, int is_background)
{
	if (count <= 0)
	{
		return; // Undefined behavior - edge case
	}
	pid_t pid = fork();
	if (pid == NOT_FOUND)
	{
		perror(strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (pid == 0)
	{
		// printf("Began execution: %s\n", arglist[0]);
		if (execvp(arglist[0], arglist) == NOT_FOUND)
		{
			perror(strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		// We only wait for foreground processes
		if (!is_background)
		{
			int status;
			wait(&status); // FIXME: int -> pid_t?
			if (WIFEXITED(status))
			{
				// printf("Child finished\n");
			}
		}
	}
}

void execute_pipe_child(char **arglist, int *fd, int is_out)
{
	int used_end = is_out;
	int other_end = !is_out; // We close the unused end of the pipe first
	int redirect_src = is_out ? STDOUT_FILENO : STDIN_FILENO;
	close(fd[other_end]);
	dup2(fd[used_end], redirect_src); // Redirect stdout to write end / stdin to read end
	close(fd[used_end]);
	if (execvp(arglist[0], arglist) == NOT_FOUND)
	{
		perror(strerror(errno));
		exit(EXIT_FAILURE);
	}
}

void execute_redirect_child(int count, char **arglist)
{
	int fd = open(arglist[count - 1], O_RDONLY);
	if (fd == NOT_FOUND)
	{
		perror(strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (dup2(fd, STDIN_FILENO) == NOT_FOUND)
	{
		perror(strerror(errno));
		exit(EXIT_FAILURE);
	}
	close(fd);
	arglist[count - 2] = NULL;
	if (execvp(arglist[0], arglist) == NOT_FOUND)
	{
		perror(strerror(errno));
		exit(EXIT_FAILURE);
	}
}

// TODO: add sigint handler