#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_TOKENS 128
#define MAX_PIPES 16
#define MAX_JOBS 64

typedef struct {
    pid_t pid;
    char cmd[256];
} Job;

Job jobs[MAX_JOBS];
int job_count = 0;

void sigint_handler(int sig) {
    write(STDOUT_FILENO, "\nUse 'exit' to quit.\n", 21);
}

void add_job(pid_t pid, char *cmd) {
    if (job_count < MAX_JOBS) {
        jobs[job_count].pid = pid;
        strncpy(jobs[job_count].cmd, cmd, 255);
        job_count++;
    }
}

void check_jobs() {
    for (int i = 0; i < job_count; i++) {
        int status;
        if (waitpid(jobs[i].pid, &status, WNOHANG) > 0) {
            printf("[done] %s (%d)\n", jobs[i].cmd, jobs[i].pid);
            for (int j = i; j < job_count - 1; j++) jobs[j] = jobs[j+1];
            job_count--;
            i--;
        }
    }
}

char **tokenize(char *line) {
    char **tokens = malloc(MAX_TOKENS * sizeof(char *));
    int i = 0;
    char *tok = strtok(line, " \t\n");
    while (tok) {
        tokens[i++] = tok;
        tok = strtok(NULL, " \t\n");
    }
    tokens[i] = NULL;
    return tokens;
}

int split_pipes(char *line, char **parts) {
    int count = 0;
    parts[count++] = strtok(line, "|");
    while ((parts[count++] = strtok(NULL, "|")));
    return count - 1;
}

void redirection(char **tokens) {
    for (int i = 0; tokens[i]; i++) {
        // output truncate >
        if (!strcmp(tokens[i], ">")) {
            int fd = open(tokens[i+1], O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (fd < 0) { perror("open"); exit(1); }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            tokens[i] = NULL;       // cut the rest
            break;
        }
        // output append >>
        if (!strcmp(tokens[i], ">>")) {
            int fd = open(tokens[i+1], O_CREAT | O_WRONLY | O_APPEND, 0644);
            if (fd < 0) { perror("open"); exit(1); }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            tokens[i] = NULL;
            break;
        }
        // input <
        if (!strcmp(tokens[i], "<")) {
            int fd = open(tokens[i+1], O_RDONLY);
            if (fd < 0) { perror("open"); exit(1); }
            dup2(fd, STDIN_FILENO);
            close(fd);
            tokens[i] = NULL;
            break;
        }
    }
}


void run_cmd(char **tokens, int background, char *rawcmd) {
    if (!tokens[0]) return;

    if (!strcmp(tokens[0], "cd")) {
        if (!tokens[1]) chdir(getenv("HOME"));
        else chdir(tokens[1]);
        return;
    }

    if (!strcmp(tokens[0], "pwd")) {
        char cwd[256];
        getcwd(cwd, sizeof(cwd));
        printf("%s\n", cwd);
        return;
    }

    if (!strcmp(tokens[0], "history")) {
        HIST_ENTRY **hist = history_list();
        if (hist) {
            for (int i = 0; hist[i]; i++)
                printf("%d  %s\n", i+1, hist[i]->line);
        }
        return;
    }

    if (!strcmp(tokens[0], "jobs")) {
        for (int i = 0; i < job_count; i++)
            printf("[%d] %s (%d)\n", i+1, jobs[i].cmd, jobs[i].pid);
        return;
    }

    if (!strcmp(tokens[0], "fg")) {
        if (job_count == 0) return;
        pid_t pid = jobs[job_count-1].pid;
        waitpid(pid, NULL, 0);
        job_count--;
        return;
    }

    if (!strcmp(tokens[0], "exit")) exit(0);

    pid_t pid = fork();
    if (pid == 0) {
        redirection(tokens);
        execvp(tokens[0], tokens);
        perror("exec");
        exit(1);
    }

    if (background) {
        add_job(pid, rawcmd);
        printf("[BG] %d started\n", pid);
    } else {
        waitpid(pid, NULL, 0);
    }
}

void run_pipeline(char *line, int background) {
    char *parts[MAX_PIPES];
    int pipes = split_pipes(line, parts);

    int in_fd = STDIN_FILENO;
    int fd[2];

    for (int i = 0; i < pipes; i++) {
        pipe(fd);
        pid_t pid = fork();

        if (pid == 0) {
            dup2(in_fd, STDIN_FILENO);
            dup2(fd[1], STDOUT_FILENO);
            close(fd[0]);
            char **tokens = tokenize(parts[i]);
            redirection(tokens);
            execvp(tokens[0], tokens);
            perror("pipe exec");
            exit(1);
        }

        waitpid(pid, NULL, 0);
        close(fd[1]);
        in_fd = fd[0];
    }

    char **tokens = tokenize(parts[pipes]);
    pid_t pid = fork();
    if (pid == 0) {
        dup2(in_fd, STDIN_FILENO);
        redirection(tokens);
        execvp(tokens[0], tokens);
        perror("pipe last exec");
        exit(1);
    }

    waitpid(pid, NULL, 0);
}

int main() {
    signal(SIGINT, sigint_handler);

    while (1) {
        check_jobs();

        char cwd[256]; getcwd(cwd, sizeof(cwd));
        char *line = readline("\033[1;32msuperShell\033[0m:\033[1;34m~\033[0m$ ");

        if (!line) break;
        if (*line) add_history(line);

        int background = 0;
        if (line[strlen(line)-1] == '&') {
            background = 1;
            line[strlen(line)-1] = '\0';
        }

        if (strchr(line, '|')) run_pipeline(line, background);
        else {
            char **tokens = tokenize(line);
            run_cmd(tokens, background, line);
            free(tokens);
        }

        free(line);
    }
}
