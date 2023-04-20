#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define STDIN 0
#define RUN_IN_BACKGROUND '&'
#define PIPE '|'
#define REDIRECT '<'
#define TRUE 1
#define SUCCESS 0
#define NOT_FOUND -1

// arglist - a list of char* arguments (words) provided by the user
// it contains count+1 items, where the last item (arglist[count]) and *only* the last is NULL
// RETURNS - 1 if should continue, 0 otherwise
int process_arglist(int count, char **arglist);

// prepare and finalize calls for initialization and destruction of anything required
int prepare(void);
int finalize(void);

int main(void)
{
	if (prepare() != 0)
		exit(1);

	while (1)
	{
		char **arglist = NULL;
		char *line = NULL;
		size_t size;
		int count = 0;

		if (getline(&line, &size, stdin) == -1)
		{
			free(line);
			break;
		}

		arglist = (char **)malloc(sizeof(char *));
		if (arglist == NULL)
		{
			printf("malloc failed: %s\n", strerror(errno));
			exit(1);
		}
		arglist[0] = strtok(line, " \t\n");

		while (arglist[count] != NULL)
		{
			++count;
			arglist = (char **)realloc(arglist, sizeof(char *) * (count + 1));
			if (arglist == NULL)
			{
				printf("realloc failed: %s\n", strerror(errno));
				exit(1);
			}

			arglist[count] = strtok(NULL, " \t\n");
		}

		if (count != 0)
		{
			if (!process_arglist(count, arglist))
			{
				free(line);
				free(arglist);
				break;
			}
		}

		free(line);
		free(arglist);
	}

	if (finalize() != 0)
		exit(1);

	return 0;
}

int prepare(void)
{
	return SUCCESS;
}

int process_arglist(int count, char **arglist)
{
	int is_background = *(*arglist + count - 1) == RUN_IN_BACKGROUND;

	if (is_background)
	{
		// TODO: implement
	}

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

	return 1;
}

int finalize(void)
{
	return SUCCESS;
}

int handle_pipe(int count, char **arglist)
{
	// First and last word cannot be pipes.
	for (int i = 1; i < count - 1; i++)
	{
		// We assume at most 1 pipe, so we return after execution.
		if (*(*arglist + i) == PIPE)
		{
			return execute_pipe(count, arglist, i);
		}
	}
	return NOT_FOUND;
}

int handle_redirect(int count, char **arglist)
{
	// If there is a redirect, it is guaranteed to be the second-to-last word.
	if (count > 2 && *(*arglist + count - 2) == REDIRECT)
	{
		return execute_redirect(count, arglist);
	}
	return NOT_FOUND;
}

int execute_pipe(int count, char **arglist, int pipe_index)
{
	return;
}

int execute_redirect(int count, char **arglist)
{
	char *pathname = *arglist + count - 1;
	int fd = open(pathname); // TODO: check whether to add O_RDONLY from fcntl.h
	if (fd == NOT_FOUND)
	{
		perror(strerror(errno));
		exit(1);
	}
	if (dup2(fd, STDIN) == NOT_FOUND)
	{
		perror(strerror(errno));
		exit(1);
	}

	char *terminator = pathname - 1;
	*terminator = NULL; // FIXME: prone to errors, make sure it's okay
	execute_command(count - 2, arglist);

	return SUCCESS;
}

int execute_command(int count, char **arglist)
{
	if (count <= 0)
	{
		return 1;
	}
	// TODO: implement

	execvp(arglist[0], arglist);

	return;
}
