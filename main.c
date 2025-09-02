#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>

int	ft_strlen(char* str)
{
	int	i = 0;

	if (!str)
		return 0;
	while (str[i])
		i++;
	return i;
}

void	ft_error(char* msg, char* cmd)
{
	write(2, msg, ft_strlen(msg));
	if (cmd)
		write(2, cmd, ft_strlen(cmd));
	write(2, "\n", 1);
}

void	ft_cd(char** av, int start, int stop)
{
	if (!av[start+1] || start+2 != stop)
		ft_error("error: cd: bad arguments", NULL);
	if (chdir(av[start+1]) == -1)
		ft_error("error: cd: cannot change directory to ", av[start+1]);
}

void	ft_pipeline_child(int save_stdin, int pipe_init[2], char** av, int start, int i, char** env)
{
	if (close(save_stdin) != 0)
	{
		ft_error("error: fatal", NULL);
		exit(EXIT_FAILURE);
	}
	if (close(pipe_init[0]) != 0)
	{
		ft_error("error: fatal", NULL);
		exit(EXIT_FAILURE);
	}
	if (dup2(pipe_init[1], STDOUT_FILENO) == -1)
	{
		ft_error("error: fatal", NULL);
		exit(EXIT_FAILURE);
	}
	if (close(pipe_init[1]) != 0)
	{
		ft_error("error: fatal", NULL);
		exit(EXIT_FAILURE);
	}
	av[i] = NULL;
	if (execve(av[start], &av[start], env) == -1)
		ft_error("error: cannot execute ", av[start]);
}

void	ft_one_child(char** av, int start, int stop, char** env)
{
	pid_t	pid;

	pid = fork();
	if (pid == -1)
	{
		ft_error("error: fatal", NULL);
		exit(EXIT_FAILURE);
	}
	if (pid == 0)
	{
		av[stop] = NULL;
		if (execve(av[start], &av[start], env) == -1)
			ft_error("error: cannot execute ", av[start]);
	}
	if (waitpid(pid, NULL, 0) == -1)
	{
		ft_error("error: fatal", NULL);
		exit(EXIT_FAILURE);
	}
}

void	ft_parent_after_child(int pipe_init[2])
{
	if (waitpid(-1, NULL, 0) == -1)
	{
		ft_error("error: fatal", NULL);
		exit(EXIT_FAILURE);
	}
	if (dup2(pipe_init[0], STDIN_FILENO) == -1)
	{
		ft_error("error: fatal", NULL);
		exit(EXIT_FAILURE);
	}
	if (close(pipe_init[0]) != 0)
	{
		ft_error("error: fatal", NULL);
		exit(EXIT_FAILURE);
	}
	if (close(pipe_init[1]) != 0)
	{
		ft_error("error: fatal", NULL);
		exit(EXIT_FAILURE);
	}
}

void	ft_put_real_stdin_back(int save_stdin)
{
	if (dup2(save_stdin, STDIN_FILENO) == -1)
	{
		ft_error("error: fatal", NULL);
		exit(EXIT_FAILURE);
	}
	if (close(save_stdin) != 0)
	{
		ft_error("error: fatal", NULL);
		exit(EXIT_FAILURE);
	}
}

void	ft_pipeline(char** av, char** env, int start, int stop)
{
	int	i = start;
	int	pipe_init[2];
	pid_t	pid;
	int	save_stdin = dup(STDIN_FILENO);

	if (save_stdin == -1)
	{
		ft_error("error: fatal", NULL);
		exit(EXIT_FAILURE);
	}
	while (i < stop && av[i])
	{
		if (strcmp(av[i], "|") == 0) //executing commands between pipes
		{
			if (pipe(pipe_init) == -1)
			{
				ft_error("error: fatal", NULL);
				exit(EXIT_FAILURE);
			}
			pid = fork();
			if (pid == -1)
			{
				ft_error("error: fatal", NULL);
				exit(EXIT_FAILURE);
			}
			if (pid == 0)
				ft_pipeline_child(save_stdin, pipe_init, av, start, i, env);
			ft_parent_after_child(pipe_init); //waiting for child; change STDIN to pipe to get the input from the output of previous cmd
			start = i+1;			
		}
		i++;
	}
	if (start < stop) //last cmd after "|" no need to write to pipe - write directly to STDOUT
		ft_one_child(av, start, stop, env);
	ft_put_real_stdin_back(save_stdin);
}

void	ft_execution(char** av, char** env, int start, int stop, int pipe)
{

	if (strcmp(av[start], ";") == 0)
		return ;
	if (pipe == 0) //no pipe so either one child process to execve either built-in "cd"
	{
		if (strcmp(av[start], "cd") == 0)
			ft_cd(av, start, stop);
		else
			ft_one_child(av, start, stop, env);
	}
	else //cmd with pipes
		ft_pipeline(av, env, start, stop);
}

void	ft_microshell(char** av, char** env)
{
	int	i = 0;
	int	pipe = 0;
	int	start = 0;

	while (av[i])
	{
		if (strcmp(av[i], "|") == 0)
			pipe = 1; //to warn the execution about pipeline
		if (strcmp(av[i], ";") == 0)
		{
			ft_execution(av, env, start, i, pipe); //exec block of cmd before ";"

			start = i+1; //to restart the next cmd
			pipe = 0;
		}
		i++;
	}
	if (start < i)
		ft_execution(av, env, start, i, pipe); //last block after ";"
}

int main(int ac, char**av, char** env)
{
	if (ac < 2)
		return 1;
	ft_microshell(&av[1], env);
}
