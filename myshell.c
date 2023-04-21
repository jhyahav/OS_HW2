#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

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
void clean_arglist(int count, char **arglist);

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

	arglist[count - 1] = NULL; // FIXME: prone to errors, make sure it's okay
	printf("Background\n");
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
	int count_out = pipe_index;
	int count_in = count - pipe_index - 1; // TODO: ensure no off-by-one here
	arglist[pipe_index] = NULL;
	char **args_out = arglist;
	char **args_in = arglist + pipe_index + 1;

	// TODO: add command execution logic w/ syscalls pipe and dup

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
	else if (pid == 0)
	{
		// printf("Began execution: %s\n", arglist[0]);
		int fd = open(arglist[count - 1], O_RDONLY);
		// printf("Opened\n");
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

		// int i = 0;
		// while (*arglist != NULL && i < count)
		// {
		// 	printf(*(arglist + i++));

		// }

		// execvp(arglist[0], arglist); // FIXME: wrap with error handling
	}
	else
	{

		int status;
		int pid = wait(&status);
		if (WIFEXITED(status))
		{
			// printf("Child finished\n");
		}
	}

	// arglist[count - 2] = NULL;
	// execute_command(count, arglist, FALSE);
	// clean_arglist(count, arglist);
	// execute_command(count - 1, arglist, FALSE);

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
	else if (pid == 0)
	{
		// printf("Began execution: %s\n", arglist[0]);
		execvp(arglist[0], arglist); // FIXME: wrap with error handling
	}
	else
	{
		// We only wait for foreground processes
		if (!is_background)
		{
			int status;
			int pid = wait(&status);
			if (WIFEXITED(status))
			{
				// printf("Child finished\n");
			}
		}
	}
}

// void clean_arglist(int count, char **arglist)
// {
// 	int i = 0;
// 	for (int j = 0; j < count; j++)
// 	{
// 		if (arglist[j][0] == REDIRECT)
// 		{
// 			continue;
// 		}
// 		printf(arglist[i]);
// 		arglist[i++] = arglist[j];
// 	}
// }