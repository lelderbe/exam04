/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lelderbe <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/09/18 15:23:26 by lelderbe          #+#    #+#             */
/*   Updated: 2021/09/18 15:23:28 by lelderbe         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define CD_WRONG_ARGS	"error: cd: bad arguments"
#define CD_FAILED		"error: cd: cannot change directory to "
#define ERR_FATAL		"error: fatal"
#define EXECVE_FAILED	"error: cannot execute "

typedef struct	s_app {
	int		fd0;
	int		fd1;
	int		pipe[2];
}				t_app;

int		ft_strlen(char *s) {
	int	count;

	count = 0;
	while (s && s[count])
		count++;
	return (count);
}

void	psyserr() {
	write(2, ERR_FATAL, ft_strlen(ERR_FATAL));
	exit(1);
}

void	perr(char *s1, char *s2) {
	write(2, s1, ft_strlen(s1));
	write(2, s2, ft_strlen(s2));
	write(2, "\n", 1);
}

void	exec_cd(t_app *e, char **argv, int count) {
	pid_t	pid;

	(void)e;
	if (argv[0] && argv[1] && !argv[2]) {
		if (count != 1) {
			// pipe
			if ((pid = fork()) == -1)
				psyserr();
			if (pid == 0) {
				//child
				if (chdir(argv[1]) == -1) {
					perr(CD_FAILED, argv[1]);
					exit(1);
				}
				exit(0);
			} else {
				// parent
			}
		} else {
			// not pipe
			if (chdir(argv[1]) == -1)
				perr(CD_FAILED, argv[1]);
		}
	} else {
		perr(CD_WRONG_ARGS, "");
	}
}

void	exec_command(t_app *e, char **argv, char **envp) {
	pid_t	pid;

	pid = fork();
	if (pid == -1)
		psyserr();
	if (pid == 0) {
		// close pipes
		close(e->fd0);
		close(e->fd1);
		close(e->pipe[0]);
		close(e->pipe[1]);

		// child
		execve(argv[0], argv, envp);
		perr(EXECVE_FAILED, argv[0]);
		exit(1);
	} else {
		// parent
	}
}

void exec(t_app *e, char ***jobs, char **envp) {
	int		j;
	char	**argv;
	int		count;

	count = 0;
	while (jobs && jobs[count])
		count++;

	//save fds
	e->fd0 = dup(STDIN_FILENO);
	e->fd1 = dup(STDOUT_FILENO);

	j = 0;
	while (jobs && jobs[j]) {
		argv = jobs[j];

		if (!argv[0])
			break ;

		// create a pipe and redir fd1
		pipe(e->pipe);
		if (count - j > 1)
			dup2(e->pipe[1], STDOUT_FILENO);
		else
			dup2(e->fd1, STDOUT_FILENO);

		if (strcmp(argv[0], "cd") == 0)
			exec_cd(e, argv, count);
		else
			exec_command(e, argv, envp);

		// redir fd0 and close pipe
		dup2(e->pipe[0], STDIN_FILENO);
		close(e->pipe[0]);
		close(e->pipe[1]);
		
		free(argv);
		j++;
	}

	// restore fds
	dup2(e->fd0, STDIN_FILENO);
	dup2(e->fd1, STDOUT_FILENO);
	close(e->fd0);
	close(e->fd1);

	//wait forks
	while (waitpid(-1, 0, 0) != -1) ;
}

int	main(int argc, char **argv, char **envp) {
	char	***jobs;
	char	**jargv;
	int		i;
	int		j;
	int		k;
	int		start;
	t_app	e = {0};

	if (!(jobs = malloc(sizeof(*jobs) * argc)))
		psyserr();
	start = 0;
	i = 1;
	j = 0;
	while (argv[i]) {
		if (!(jargv = malloc(sizeof(*jargv) * argc)))
			psyserr();
		jobs[j] = jargv;
		k = 0;
		while (argv[i]) {
			if (strcmp(argv[i], "|") == 0 || strcmp(argv[i], ";") == 0) {
				i++;
				break ;
			}
			jargv[k] = argv[i];
			k++;
			i++;
		}
		j++;
		jargv[k] = 0;
		jobs[j] = 0;
		if (strcmp(argv[i - 1], ";") == 0) {
			// exec and shift start
			exec(&e, jobs + start, envp);
			start = j;
		}
	}

	exec(&e, jobs + start, envp);

	free(jobs);

	//while (1);

	return (0);
}
