/* Deepseek AI generated, non-human creation! */
/* Ported to ANSI C standard */


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <stdlib.h>
#include <termios.h>
#include <pwd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#define MAXLINE 256
#define MAXARGS 32
#define MAXHIST 100
#define MAXPATH 512
#define HIST_FILE ".dsh_history"

struct termios oldt, newt;

char history[MAXHIST][MAXLINE];
int hsize, hpos, hist_current;
char best_match[MAXLINE];
int interrupted;

/* Function prototypes */
void getty(void);
void resetty(void);
char *readln(void);
void hist_add(char *s);
void hist_show(void);
void hist_load(void);
void hist_save(void);
void complete(char *prefix);
int common_prefix(char *a, char *b);
void move_cursor_left(int n);
void move_cursor_right(int n);
void clear_line(void);
void show_help(void);
void execute_command(char *cmdline);
void execute_redirection(char **args);
void execute_pipeline(char *cmdline);
void sigint_handler(int sig);
void print_prompt(void);
char *my_getcwd(void);
void hist_init(void);
void show_usage(void);
int is_shell_script(char *filename);

char *my_getcwd(void)
{
    static char cwd[MAXPATH];
    int pfd[2];
    int pid;
    int n;
    
    if (pipe(pfd) < 0)
        return "?";
    
    pid = fork();
    if (pid == 0) {
        close(pfd[0]);
        dup2(pfd[1], 1);
        close(pfd[1]);
        execl("/bin/pwd", "pwd", NULL);
        execl("/usr/bin/pwd", "pwd", NULL);
        exit(1);
    }
    else if (pid > 0) {
        close(pfd[1]);
        n = read(pfd[0], cwd, MAXPATH - 1);
        close(pfd[0]);
        wait((int*)NULL);
        
        if (n > 0) {
            if (cwd[n-1] == '\n')
                cwd[n-1] = '\0';
            else
                cwd[n] = '\0';
            return cwd;
        }
    }
    
    return "?";
}

void sigint_handler(int sig)
{
    interrupted = 1;
    write(1, "\n", 1);
}

void getty(void)
{
    ioctl(0, TCGETS, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    newt.c_cc[VMIN] = 1;
    newt.c_cc[VTIME] = 0;
    ioctl(0, TCSETS, &newt);
}

void resetty(void)
{
    ioctl(0, TCSETS, &oldt);
}

void move_cursor_left(int n)
{
    while (n-- > 0)
        write(1, "\b", 1);
}

void move_cursor_right(int n)
{
    int i;
    for (i = 0; i < n; i++)
        write(1, "\033[C", 3);
}

void clear_line(void)
{
    write(1, "\033[K", 3);
}

void hist_init(void)
{
    char path[MAXPATH];
    char *home;
    
    home = getenv("HOME");
    if (home == NULL)
        return;
    
    sprintf(path, "%s/%s", home, HIST_FILE);
    unlink(path);
    
    printf("History file initialized (deleted)\n");
}

void hist_load(void)
{
    char path[MAXPATH];
    char *home;
    int fd;
    char line[MAXLINE];
    int n, i;
    char *space;
    char *cmd_start;
    
    hsize = 0;
    home = getenv("HOME");
    if (home == NULL)
        return;
    
    sprintf(path, "%s/%s", home, HIST_FILE);
    fd = open(path, 0);
    if (fd < 0)
        return;
    
    while (hsize < MAXHIST) {
        n = 0;
        while (n < MAXLINE - 1) {
            if (read(fd, &line[n], 1) != 1) {
                if (n == 0) {
                    close(fd);
                    return;
                }
                break;
            }
            if (line[n] == '\n')
                break;
            n++;
        }
        line[n] = '\0';
        
        if (n == 0)
            break;
        
        space = strchr(line, ' ');
        if (space != NULL) {
            cmd_start = space + 1;
            while (*cmd_start == ' ')
                cmd_start++;
            
            strcpy(history[hsize], cmd_start);
            
            n = strlen(history[hsize]);
            if (n > 0 && history[hsize][n-1] == '\n')
                history[hsize][n-1] = '\0';
            
            if (history[hsize][0] != '\0') {
                if (hsize == 0 || strcmp(history[hsize], history[hsize-1]) != 0)
                    hsize++;
            }
        }
    }
    close(fd);
    
    hpos = hsize;
    hist_current = hsize;
}

void hist_save(void)
{
    char path[MAXPATH];
    char *home;
    int fd, i;
    char temp_file[MAXPATH];
    char line[MAXLINE];
    
    home = getenv("HOME");
    if (home == NULL)
        return;
    
    sprintf(path, "%s/%s", home, HIST_FILE);
    sprintf(temp_file, "%s/%s.tmp", home, HIST_FILE);
    
    fd = open(temp_file, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0)
        return;
    
    for (i = 0; i < hsize; i++) {
        if (history[i][0] != '\0') {
            sprintf(line, "%d %s\n", i+1, history[i]);
            write(fd, line, strlen(line));
        }
    }
    close(fd);
    
    unlink(path);
    link(temp_file, path);
    unlink(temp_file);
}

void hist_add(char *s)
{
    int i;
    
    if (s[0] == '\0')
        return;
    
    if (hsize > 0 && strcmp(s, history[hsize-1]) == 0)
        return;
    
    if (strcmp(s, "history") == 0)
        return;
    
    if (hsize >= MAXHIST) {
        for (i = 1; i < MAXHIST; i++)
            strcpy(history[i-1], history[i]);
        hsize = MAXHIST - 1;
    }
    strcpy(history[hsize++], s);
    hpos = hsize;
    hist_current = hsize;
}

void hist_show(void)
{
    int i;
    if (hsize == 0) {
        printf("No commands in history\n");
        return;
    }
    for (i = 0; i < hsize; i++) {
        if (history[i][0] != '\0')
            printf("%3d  %s\n", i+1, history[i]);
    }
}

int common_prefix(char *a, char *b)
{
    int i = 0;
    while (a[i] && b[i] && a[i] == b[i])
        i++;
    return i;
}

void complete(char *prefix)
{
    char *path;
    char dir[MAXPATH], base[MAXLINE];
    char *slash;
    DIR *d;
    struct dirent *de;
    char first[MAXLINE];
    int found;
    char fullpath[MAXPATH];

    best_match[0] = '\0';
    found = 0;

    if (strchr(prefix, '/')) {
        strcpy(fullpath, prefix);

        slash = strrchr(fullpath, '/');
        if (slash != NULL) {
            *slash = '\0';
            strcpy(base, slash + 1);
            strcpy(dir, fullpath);
            if (dir[0] == '\0')
                strcpy(dir, "/");
        } else {
            strcpy(base, fullpath);
            strcpy(dir, ".");
        }

        d = opendir(dir);
        if (d == NULL)
            return;

        while ((de = readdir(d)) != NULL) {
            if (de->d_name[0] == '.' && base[0] != '.')
                continue;
            if (!strncmp(de->d_name, base, strlen(base))) {
                if (!found) {
                    strcpy(first, de->d_name);
                    found = 1;
                } else {
                    first[common_prefix(first, de->d_name)] = '\0';
                }
            }
        }
        closedir(d);
        if (found) {
            if (slash != NULL) {
                if (strcmp(dir, "/") == 0)
                    sprintf(best_match, "/%s", first);
                else
                    sprintf(best_match, "%s/%s", dir, first);
            } else {
                strcpy(best_match, first);
            }
        }
        return;
    }

    path = getenv("PATH");
    if (path == NULL)
        return;

    while (*path) {
        char *end = strchr(path, ':');
        int len = end ? (end - path) : strlen(path);

        if (len > 0 && len < MAXPATH) {
            strncpy(dir, path, len);
            dir[len] = '\0';

            d = opendir(dir);
            if (d != NULL) {
                while ((de = readdir(d)) != NULL) {
                    if (de->d_name[0] == '.' && prefix[0] != '.')
                        continue;
                    if (!strncmp(de->d_name, prefix, strlen(prefix))) {
                        if (!found) {
                            strcpy(first, de->d_name);
                            found = 1;
                        } else {
                            first[common_prefix(first, de->d_name)] = '\0';
                        }
                    }
                }
                closedir(d);
            }
        }
        if (end == NULL)
            break;
        path = end + 1;
    }
    if (found)
        strcpy(best_match, first);
}

void print_prompt(void)
{
    char hostname[MAXLINE];
    char *cwd;
    char *user;
    struct passwd *pwd;
    int uid;
    
    uid = getuid();
    if (uid == 0) {
        user = "root";
    } else {
        pwd = getpwuid(uid);
        user = (pwd != NULL) ? pwd->pw_name : "unknown";
    }
    
    if (gethostname(hostname, MAXLINE) < 0)
        strcpy(hostname, "unknown");
    
    cwd = my_getcwd();
    if (cwd == NULL || cwd[0] == '\0')
        cwd = "?";
    
    printf("[%s@%s %s]%c ", user, hostname, cwd, 
           (uid == 0) ? '#' : '$');
    fflush(stdout);
}

char *readln(void)
{
    static char buf[MAXLINE];
    int pos, len;
    char c;
    char seq1, seq2;
    int i, start;
    char tmp[MAXLINE];
    int old_len;
    int old_len_word;
    int new_len_word;

    memset(buf, '\0', MAXLINE);
    pos = 0;
    len = 0;
    hpos = hsize;
    hist_current = hsize;

    print_prompt();

    while (1) {
        if (read(0, &c, 1) <= 0)
            return 0;

        if (c == '\n') {
            write(1, "\n", 1);
            return buf;
        }

        if (c == 1) { /* Ctrl-A */
            while (pos > 0) {
                pos--;
                move_cursor_left(1);
            }
            continue;
        }

        if (c == 5) { /* Ctrl-E */
            while (pos < len) {
                pos++;
                write(1, "\033[C", 3);
            }
            continue;
        }

        if (c == 127 || c == '\b') { /* Backspace */
            if (pos > 0) {
                /* Move cursor left */
                move_cursor_left(1);
                /* Shift characters left */
                for (i = pos; i < len; i++)
                    buf[i-1] = buf[i];
                len--;
                pos--;
                /* Write the rest of the line */
                write(1, buf + pos, len - pos);
                /* Add space and move back */
                write(1, " ", 1);
                for (i = 0; i < len - pos + 1; i++)
                    move_cursor_left(1);
                buf[len] = '\0';
            }
            continue;
        }

        if (c == 27) { /* Escape sequence */
            if (read(0, &seq1, 1) != 1) continue;
            if (read(0, &seq2, 1) != 1) continue;

            if (seq2 == 'A') { /* Up arrow */
                if (hist_current > 0) {
                    /* Clear current line */
                    for (i = 0; i < len; i++)
                        write(1, "\b \b", 3);
                    len = 0;
                    pos = 0;
                    
                    hist_current--;
                    strcpy(buf, history[hist_current]);
                    len = strlen(buf);
                    write(1, buf, len);
                    pos = len;
                }
                continue;
            }

            if (seq2 == 'B') { /* Down arrow */
                /* Clear current line */
                for (i = 0; i < len; i++)
                    write(1, "\b \b", 3);
                len = 0;
                pos = 0;
                
                if (hist_current < hsize - 1) {
                    hist_current++;
                    strcpy(buf, history[hist_current]);
                    len = strlen(buf);
                    write(1, buf, len);
                    pos = len;
                } else if (hist_current < hsize) {
                    hist_current = hsize;
                    buf[0] = '\0';
                    len = 0;
                    pos = 0;
                }
                continue;
            }

            if (seq2 == 'D') { /* Left arrow */
                if (pos > 0) {
                    pos--;
                    move_cursor_left(1);
                }
                continue;
            }

            if (seq2 == 'C') { /* Right arrow */
                if (pos < len) {
                    pos++;
                    write(1, "\033[C", 3);
                }
                continue;
            }
        }

        if (c == 9) { /* Tab completion */
            /* Find start of current word */
            i = pos - 1;
            while (i >= 0 && buf[i] != ' ' && buf[i] != '\t' &&
                   buf[i] != ';' && buf[i] != '|' && buf[i] != '&')
                i--;
            start = i + 1;
            
            /* Copy current word */
            strncpy(tmp, buf + start, sizeof(tmp) - 1);
            tmp[sizeof(tmp) - 1] = '\0';
            
            /* Get completion */
            complete(tmp);
            
            if (best_match[0] != '\0') {
                old_len_word = strlen(tmp);
                new_len_word = strlen(best_match);
                
                /* Move cursor to start of word */
                for (i = 0; i < pos - start; i++)
                    move_cursor_left(1);
                
                /* Calculate new line length */
                old_len = len;
                len = len - old_len_word + new_len_word;
                
                /* Update buffer */
                if (new_len_word > old_len_word) {
                    /* Need to shift right */
                    for (i = len; i >= start + new_len_word; i--)
                        buf[i] = buf[i - (new_len_word - old_len_word)];
                } else if (new_len_word < old_len_word) {
                    /* Need to shift left */
                    for (i = start + new_len_word; i <= len; i++)
                        buf[i] = buf[i + (old_len_word - new_len_word)];
                }
                
                /* Copy the completed word */
                strncpy(buf + start, best_match, new_len_word);
                
                /* Write the new word */
                write(1, best_match, new_len_word);
                
                /* Write the rest of the line */
                write(1, buf + start + new_len_word, len - (start + new_len_word));
                
                /* Position cursor at end of completed word */
                pos = start + new_len_word;
                
                /* Move cursor back if needed */
                for (i = 0; i < len - pos; i++)
                    move_cursor_left(1);
            }
            continue;
        }

        if (c >= 32 && c < 127 && len < MAXLINE - 1) {
            /* Insert character at cursor position */
            if (pos < len) {
                /* Shift right */
                for (i = len; i > pos; i--)
                    buf[i] = buf[i-1];
            }
            buf[pos] = c;
            pos++;
            len++;
            buf[len] = '\0';
            
            /* Write the character and rest of line */
            write(1, &c, 1);
            write(1, buf + pos, len - pos);
            
            /* Move cursor back to position */
            for (i = 0; i < len - pos; i++)
                move_cursor_left(1);
            continue;
        }
    }
}

void execute_redirection(char **args)
{
    int i, fd;
    char *infile = NULL, *outfile = NULL;
    int append = 0;

    for (i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "<") == 0) {
            infile = args[i+1];
            args[i] = NULL;
            i++;
        }
        else if (strcmp(args[i], ">") == 0) {
            outfile = args[i+1];
            append = 0;
            args[i] = NULL;
            i++;
        }
        else if (strcmp(args[i], ">>") == 0) {
            outfile = args[i+1];
            append = 1;
            args[i] = NULL;
            i++;
        }
    }

    if (infile != NULL) {
        fd = open(infile, O_RDONLY);
        if (fd < 0) {
            perror("dsh");
            return;
        }
        dup2(fd, 0);
        close(fd);
    }

    if (outfile != NULL) {
        if (append)
            fd = open(outfile, O_WRONLY | O_CREAT | O_APPEND, 0644);
        else
            fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror("dsh");
            return;
        }
        dup2(fd, 1);
        close(fd);
    }
}

void execute_pipeline(char *cmdline)
{
    char *commands[MAXARGS];
    int cmd_count = 0;
    int pipes[19][2];  /* MAXARGS-1 = 31, but use 19 for safety */
    int i, j;
    char *cmd;
    char line_copy[MAXLINE];
    int status;

    strcpy(line_copy, cmdline);
    cmd = strtok(line_copy, "|");
    while (cmd != NULL && cmd_count < MAXARGS) {
        while (*cmd == ' ') cmd++;
        commands[cmd_count++] = cmd;
        cmd = strtok(NULL, "|");
    }

    if (cmd_count == 1) {
        execute_command(commands[0]);
        return;
    }

    for (i = 0; i < cmd_count - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            return;
        }
    }

    for (i = 0; i < cmd_count; i++) {
        if (fork() == 0) {
            if (i > 0)
                dup2(pipes[i-1][0], 0);
            if (i < cmd_count - 1)
                dup2(pipes[i][1], 1);
            
            for (j = 0; j < cmd_count - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            execute_command(commands[i]);
            exit(0);
        }
    }

    for (i = 0; i < cmd_count - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    for (i = 0; i < cmd_count; i++)
        wait(&status);
}

int is_shell_script(char *filename)
{
    int fd;
    char magic[2];
    int n;
    
    fd = open(filename, O_RDONLY);
    if (fd < 0)
        return 0;
    
    n = read(fd, magic, 2);
    close(fd);
    
    if (n == 2 && magic[0] == '#' && magic[1] == '!')
        return 1;
    
    return 0;
}

void execute_command(char *cmdline)
{
    char *args[MAXARGS];
    char *token;
    int i, pid;
    char cmd_copy[MAXLINE];
    int background = 0;
    int len;

    len = strlen(cmdline);
    if (len > 0 && cmdline[len-1] == '&') {
        background = 1;
        cmdline[len-1] = '\0';
        while (len-2 >= 0 && cmdline[len-2] == ' ')
            cmdline[--len-1] = '\0';
    }

    if (strchr(cmdline, '|') != NULL) {
        execute_pipeline(cmdline);
        return;
    }

    strcpy(cmd_copy, cmdline);

    i = 0;
    token = strtok(cmd_copy, " \t\n");
    while (token != NULL && i < MAXARGS - 1) {
        args[i++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[i] = NULL;

    if (args[0] == NULL)
        return;

    if (!strcmp(args[0], "cd")) {
        if (args[1] == NULL)
            chdir(getenv("HOME"));
        else
            chdir(args[1]);
        return;
    }

    if (!strcmp(args[0], "help")) {
        show_help();
        return;
    }

    if (!strcmp(args[0], "history")) {
        hist_show();
        return;
    }

    if (!strcmp(args[0], "man")) {
        if (args[1] != NULL && !strcmp(args[1], "dsh")) {
            show_help();
            return;
        }
    }

    if (is_shell_script(args[0])) {
        char *sh_args[MAXARGS+2];
        sh_args[0] = "/bin/sh";
        sh_args[1] = args[0];
        for (i = 1; args[i] != NULL; i++)
            sh_args[i+1] = args[i];
        sh_args[i+1] = NULL;
        
        if ((pid = fork()) == 0) {
            execute_redirection(sh_args + 1);
            execvp("/bin/sh", sh_args);
            perror("dsh");
            exit(1);
        }
        else if (pid > 0) {
            if (!background)
                wait((int*)NULL);
        }
        return;
    }

    if ((pid = fork()) == 0) {
        execute_redirection(args);
        execvp(args[0], args);
        perror("dsh");
        exit(1);
    }
    else if (pid > 0) {
        if (!background)
            wait((int*)NULL);
    }
}

void show_usage(void)
{
    printf("Usage: dsh [OPTIONS]\n");
    printf("Options:\n");
    printf("  --init         Initialize/delete history file (.dsh_history)\n");
    printf("  -h, --help     Show this help message\n");
    printf("  --version      Show version information\n");
    printf("\n");
}

void show_help(void)
{
    printf("\n");
    printf("DSH(1)                    User Commands                    DSH(1)\n\n");
    printf("NAME\n");
    printf("       dsh - Deepseek's shell, a command interpreter with line editing\n\n");
    printf("SYNOPSIS\n");
    printf("       dsh [--init]\n\n");
    printf("DESCRIPTION\n");
    printf("       Dsh is a command interpreter that executes commands read from\n");
    printf("       standard input or from a file. It features line editing, command\n");
    printf("       history, tab completion, redirection, and pipelines.\n\n");
    printf("OPTIONS\n");
    printf("       --init    Initialize the history file by deleting .dsh_history\n\n");
    printf("BUILT-IN COMMANDS\n");
    printf("       help     - Display this help message\n");
    printf("       exit     - Exit the shell\n");
    printf("       history  - Show command history\n");
    printf("       cd [dir] - Change current directory\n");
    printf("       man dsh  - Display this manual page\n\n");
    printf("LINE EDITING\n");
    printf("       Ctrl-A        - Beginning of line\n");
    printf("       Ctrl-E        - End of line\n");
    printf("       Up/Down Arrow - History navigation\n");
    printf("       Left/Right Arrow - Move cursor\n");
    printf("       Tab           - Auto-complete\n");
    printf("       Backspace     - Delete character\n");
    printf("       Ctrl-C        - Interrupt command\n\n");
    printf("REDIRECTION\n");
    printf("       cmd > file    - Redirect output\n");
    printf("       cmd >> file   - Append output\n");
    printf("       cmd < file    - Redirect input\n\n");
    printf("PIPELINES\n");
    printf("       cmd1 | cmd2   - Pipe output to input\n\n");
    printf("BACKGROUND EXECUTION\n");
    printf("       cmd &         - Execute command in background\n\n");
    printf("SHELL SCRIPTS\n");
    printf("       Shell scripts are automatically forwarded to /bin/sh\n\n");
    printf("HISTORY FILE FORMAT\n");
    printf("       History is stored in ~/.dsh_history with format:\n");
    printf("       1 command1\n");
    printf("       2 command2\n");
    printf("       3 command3\n\n");
    printf("EXAMPLES\n");
    printf("       dsh --init                 # Initialize history\n");
    printf("       ls -la | grep .c | wc -l\n");
    printf("       echo hello >> output.txt\n");
    printf("       sort < input.txt > output.txt\n");
    printf("       ./myscript.sh arg1 arg2 &\n");
    printf("       man dsh\n\n");
    printf("AUTHOR\n");
    printf("       Deepseek's Shell (dsh) Version 2.0\n\n");
}

int main(int argc, char *argv[])
{
    char *line;
    char line_copy[MAXLINE];
    int i;
    
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--init") == 0) {
            hist_init();
            return 0;
        }
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            show_usage();
            return 0;
        }
        else if (strcmp(argv[i], "--version") == 0) {
            printf("Deepseek's Shell (dsh) version 2.0\n");
            return 0;
        }
        else {
            printf("dsh: unknown option '%s'\n", argv[i]);
            show_usage();
            return 1;
        }
    }
    
    printf("Deepseek's Shell (dsh) v2.0\n");
    printf("Type 'help' for help, 'exit' to quit, 'man dsh' for manual\n");

    signal(SIGINT, sigint_handler);
    hist_load();

    while (1) {
        interrupted = 0;
        getty();
        line = readln();
        resetty();

        if (line == NULL || interrupted)
            continue;

        if (*line == '\0')
            continue;

        strcpy(line_copy, line);

        if (strcmp(line_copy, "history") != 0) {
            hist_add(line_copy);
        }

        if (!strcmp(line_copy, "exit")) {
            hist_save();
            break;
        }

        execute_command(line_copy);
    }
    return 0;
}
