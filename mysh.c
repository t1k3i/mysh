#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/utsname.h>
#include <sys/wait.h>

#define MAX_TOKENS 30
#define MAX_LINE_LENGTH 1024
#define NUM_OF_BUILTIN_COMMANDS 38
#define MAX_PATH_LENGTH 100
#define BUFSIZE 1024
#define MAX_NUM_OF_PROCESES 2048

typedef struct {
    char* i;
    char* o;
    int b;
} Options;

typedef struct {
    char* name;
    int (*fun_pointer)();
    char* help;
} Builtin_comm;

// BUILTIN COMMANDS
int debug();
int prompt();
int status();
int ex();
int help();
int prin();
int echo();
int len();
int sum();
int calc();
int basename();
int dirname();
int dirch();
int dirwd();
int dirmk();
int dirrm();
int dirls();
int ren();
int unl();
int rem();
int linkh();
int links();
int linkr();
int linkl();
int cpcat();
int pid();
int ppid();
int uid();
int euid();
int gid();
int egid();
int sysinfo();
int proc();
int pids();
int pinfo();
int waitone();
int waitall();
int pipes();

Builtin_comm commands[] = {
    {"debug", debug, "Adjusts or displays the current debugging level."},
    {"prompt", prompt, "Sets or displays the command prompt (prompt)."},
    {"status", status, "Displays the exit status of the last executed command without changing the status."},
    {"exit", ex, "Terminates the shell with a specified exit status if provided."},
    {"help", help, "Lists all supported commands and provides brief descriptions or usage guidelines."},
    {"print", prin, "Prints arguments without new line."},
    {"echo", echo, "Prints arguments with new line."},
    {"len", len, "Prints the lenght of all arguments (as strings)."},
    {"sum", sum, "Prints the sum of arguments (whole numbers)."},
    {"calc", calc, "Performs an arithmetic operation on two given arguments and outputs the result."},
    {"basename", basename, "Prints the base name of the given path arg."},
    {"dirname", dirname, "Prints the directory of the given path arg."},
    {"dirch", dirch, "Changes the current working directory."},
    {"dirwd", dirwd, "Prints the current working directory."},
    {"dirmk", dirmk, "Creates a directory."},
    {"dirrm", dirrm, "Deletes a directory."},
    {"dirls", dirls, "Displays directory contents."},
    {"rename", ren, "Renames the given file arg."},
    {"unlink", unl, "Remove the directory entry with the given name."},
    {"remove", rem, "Deleting a file or directory with the specified name."},
    {"linkhard", linkh, "Creates a hard link with the specified name to the target."},
    {"linksoft", links, "Creates a soft link with the specified name to the target."},
    {"linkread", linkr, "Prints the target of the given symbolic link."},
    {"linklist", linkl, "Searches for all hard links to a file with the specified name in the current working directory."},
    {"cpcat", cpcat, "Combines the cp and cat commands."},
    {"pid", pid, "Prints the PID of the shell process."},
    {"ppid", ppid, "Prints the PID of the parent process of the shell."},
    {"uid", uid, "Prints the UID of the user who owns the shell process."},
    {"euid", euid, "Prints the UID of the user who is the current owner of the shell process."},
    {"gid", gid, "Prints the GID of the group to which the shell process belongs."},
    {"egid", egid, "Prints the GID of the group to which the shell process currently belongs."},
    {"sysinfo", sysinfo, "Displays basic information of the system."},
    {"proc", proc, "Sets the path to the procfs file system."},
    {"pids", pids, "Displays the PIDs of current processes obtained from procfs."},
    {"pinfo", pinfo, "Displays information about current processes (PID, PPID, STATE, NAME) obtained from the stat file in procfs."},
    {"waitone", waitone, "Waits for the child process with given pid."},
    {"waitall", waitall, "Waits for all child processes."},
    {"pipes", pipes, "Creates a pipeline."}
};

char line[MAX_LINE_LENGTH];
char* tokens[MAX_TOKENS];

typedef struct {
    int p;
    int pp;
    char s;
    char ime[256];
} Pinfo;

// FUNCTIONS PROTOTIPES
int tokenize(char *s);
void print(int count);
Options* lats3();
void printOptions(Options* options);
void main_loop();
int find_builtin(char *cmd);
int execute_external(Options* options);
char* fullCommand();
void execute_builtin(int ix, Options* options);
int checkFirstArgv(char *a, int* in);
int checkSecondArgv(char *a, int *out);
int copyFromTo(int in, int out);
int compare(const void *a, const void *b);
int compare2(const void *a, const void *b);
void redirect(char* file, int fd2);
int redirect_f(char* file, int fd2);
int execute_child(char* stopnja, int fd1[2], int fd2[2]);

// GLOBAL VARIABLES
int token_count = 0;
int debug_level = 0;
char pr[9] = "mysh";
int sta = 0;
char procfsPath[512] = "/proc";

//SIGNAL HANDLERS
void sigchld_handler (int signum) {
    int pid, status, serrno;
    serrno = errno;
    while (1) {
        pid = waitpid (WAIT_ANY, &status, WNOHANG);
        if (pid < 0) {
            if (errno != ECHILD) perror ("waitpid");
            break;
        }
        if (pid == 0)
            break;
    }
    errno = serrno;
}

int main() {

    if (isatty(STDIN_FILENO)) {
        while (1) {
            printf("%s> ", pr);
            fgets(line, MAX_LINE_LENGTH, stdin);
            main_loop();
        }
    } else {
        while (fgets(line, MAX_LINE_LENGTH, stdin) != NULL) {
            main_loop();
        }
    }

    return sta;
}

void main_loop() {
    signal(SIGCHLD, sigchld_handler);
    char original[MAX_LINE_LENGTH];
    strcpy(original, line);
    if (original[strlen(original) - 1] == '\n') original [strlen(original) - 1] = '\0';
    token_count = tokenize(line);
    if (token_count == 0) {
        if (debug_level) printf("Input line: '%s'\n", original);
        fflush(stdout);
        return;
    }
    int all_tokens = token_count;
    Options* options = lats3();
    if (debug_level) {
        printf("Input line: '%s'\n", original); 
        print(all_tokens);
        printOptions(options);
        fflush(stdout);
    }
    tokens[token_count] = NULL;
    int commandIndex = find_builtin(tokens[0]);
    if (commandIndex == -1) {
        sta = execute_external(options);
        fflush(stdout);
    } else {
        execute_builtin(commandIndex, options);
    }
    free(options);
    token_count = 0;
}

int tokenize(char *s) {
    int tokenCount = 0;
    int len = strlen(s);
    int past = 0;
    int narekovaji = 0;
    while (*s != '\0') {
        if (narekovaji || !isspace(*s)) {
            if (!past && *s == '#') {
                break;
            }
            if (narekovaji && *s == '"') {
                narekovaji = 0;
                *s = '\0';
                s++;
                past = 0;
                continue;
            }
            if (*s == '"' && !past && !narekovaji) {
                narekovaji = 1;
                s++;
                continue;
            }
            if (!past) {
                past = 1;
                tokens[tokenCount++] = s;
            }
        } else {
            past = 0;
            *s = '\0';
        }
        s++;
    }
    return tokenCount;
}

Options* lats3() {
    Options* options = malloc(sizeof(options));
    options -> b = 0;
    options -> i = NULL;
    options -> o = NULL;
    int min = token_count > 3 ? token_count - 3 : 1;
    int last = 1;
    int second = 1;
    int third = 1;
    for (int i = token_count - 1; i >= min; i--) {
        if (last && strcmp(tokens[i], "&") == 0) {
            options -> b = 1;
            last = 0;
            token_count--;
            continue;
        }
        if (second && tokens[i][0] == '>') {
            options -> o = &tokens[i][1];
            last = 0;
            second = 0;
            token_count--;
            continue;
        }
        if (third && tokens[i][0] == '<') {
            options -> i = &tokens[i][1];
            last = 0;
            second = 0;
            third = 0;
            token_count--;
        }
    }
    return options;
}

void print(int count) {
    for (int i = 0; i < count; i++) {
        printf("Token %d: '%s'\n", i, tokens[i]);
    }
}

void printOptions(Options* options) {
    if (options -> i != NULL) {
        printf("Input redirect: '%s'\n", options -> i);
    }
    if (options -> o != NULL) {
        printf("Output redirect: '%s'\n", options -> o);
    }
    if (options -> b != 0) {
        printf("Background: %d\n", options -> b);
    }
}

int find_builtin(char *cmd) {
    for (int i = 0; i < NUM_OF_BUILTIN_COMMANDS; i++) {
        if (strcmp(cmd, commands[i].name) == 0) {
            return i;
        }
    }
    return -1;
}

int execute_external(Options* options) {
    char* value = fullCommand();
    //printf("External command '%s'\n", value);
    free(value);
    fflush(stdin);
    int pid = fork();
    int ex = errno;
    if (pid < 0) {
        perror("fork");
        return ex;
    } else if (pid == 0) {
        if (options -> i != NULL) {
            redirect(options -> i, STDIN_FILENO);
        }
        if (options -> o != NULL) {
            redirect(options -> o, STDOUT_FILENO);
        }
        execvp(tokens[0], tokens);
        perror("exec");
        exit(127);
    }
    if (!options -> b) {
        int s;
        if (waitpid(pid, &s, 0) == -1) {
            ex = errno;
            perror("waitpid");
            return ex;
        }
        if (WIFEXITED(s)) {
            return WEXITSTATUS(s);
        }
    }
    return 0;
}

void redirect(char* file, int fd2) {
    fflush(stdin);
    fflush(stdout);
    int fd;
    if (fd2 == STDIN_FILENO) 
        fd = open(file, O_RDONLY);
    else if (fd2 == STDOUT_FILENO)
        fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    int ex = errno;
    if (fd == -1) {
        perror(file);
        fflush(stderr);
        exit(ex);
    }
    if (dup2(fd, fd2) == -1) {
        ex = errno;
        perror("dup2");
        fflush(stderr);
        close(fd);
        exit(ex);
    }
    close(fd);
}

char* fullCommand() {
    int total_length = 0;
    for (int i = 0; i < token_count; i++) {
        total_length += strlen(tokens[i]) + 1;
    }
    char* value = malloc(total_length * sizeof(char));
    strcpy(value, tokens[0]);
    for (int i = 1; i < token_count; i++) {
        strcat(value, " ");
        strcat(value, tokens[i]);
    }
    return value;
}

void execute_builtin(int ix, Options* options) {
    if (debug_level && !options -> b) printf("Executing builtin '%s' in foreground\n", tokens[0]);
    if (debug_level && options -> b) printf("Executing builtin '%s' in background\n", tokens[0]);
    fflush(stdin);
    if (options -> b) {
        int p = fork();
        if (p < 0) {
            perror("fork");
            return;
        } else if (p == 0) {
            if (options -> i != NULL) {
                redirect(options -> i, STDIN_FILENO);
            }
            if (options -> o != NULL) {
                redirect(options -> o, STDOUT_FILENO);
            }
            int e = commands[ix].fun_pointer();
            fflush(stdout);
            exit(e);
        }
    } else {
        fflush(stdin);
        fflush(stdout);
        int fd0 = dup(STDIN_FILENO);
        int ex = errno;
        if (fd0 == -1) {
            perror("dup");
            fflush(stderr);
            sta = ex;
            return;
        }
        int fd1 = dup(STDOUT_FILENO);
        ex = errno;
        if (fd1 == -1) {
            perror("dup");
            fflush(stderr);
            sta = ex;
            close(fd0);
            return;
        }
        int fd2 = dup(STDERR_FILENO);
        ex = errno;
        if (fd2 == -1) {
            perror("dup");
            fflush(stderr);
            sta = ex;
            close(fd1);
            close(fd0);
            return;
        }
        if (options -> i != NULL) {
            int e = redirect_f(options -> i, STDIN_FILENO);
            if (e != 0) {
                sta = e;
                dup2(fd0, STDIN_FILENO);
                dup2(fd1, STDOUT_FILENO);
                dup2(fd2, STDERR_FILENO);
                close(fd0);
                close(fd1);
                close(fd2);
                return;
            } 
        }
        if (options -> o != NULL) {
            int e = redirect_f(options -> o, STDOUT_FILENO);
            if (e != 0) {
                sta = e;
                dup2(fd0, STDIN_FILENO);
                dup2(fd1, STDOUT_FILENO);
                dup2(fd2, STDERR_FILENO);
                close(fd0);
                close(fd1);
                close(fd2);
                return;
            }
        }
        sta = commands[ix].fun_pointer();
        dup2(fd0, STDIN_FILENO);
        dup2(fd1, STDOUT_FILENO);
        dup2(fd2, STDERR_FILENO);
        close(fd0);
        close(fd1);
        close(fd2);
    }
}

int redirect_f(char* file, int fd2) {
    fflush(stdin);
    fflush(stdout);
    int fd;
    if (fd2 == STDIN_FILENO) 
        fd = open(file, O_RDONLY);
    else if (fd2 == STDOUT_FILENO)
        fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    int ex = errno;
    if (fd == -1) {
        perror(file);
        fflush(stderr);
        return ex;
    }
    if (dup2(fd, fd2) == -1) {
        ex = errno;
        perror("dup2");
        fflush(stderr);
        close(fd);
        return ex;
    }
    close(fd);
    return 0;
}

int debug() {
    if (token_count > 2) return 1;
    if (token_count == 1) printf("%d\n", debug_level);
    else debug_level = atoi(tokens[1]);
    fflush(stdout);
    return 0;
}

int prompt() {
    if (token_count > 2) return 1;
    if (token_count == 1) printf("%s\n", pr);
    else {
        if (strlen(tokens[1]) > 8) return 1;
        strcpy(pr, tokens[1]);
    }
    fflush(stdout);
    return 0;
}

int status() {
    if (token_count > 1) return 1;
    printf("%d\n", sta);
    fflush(stdout);
    return sta;
}

int ex() {
    if (token_count > 2) return 1;
    if (token_count == 1) exit(sta);
    else exit(atoi(tokens[1]));
}

int help() {
    if (token_count > 1) return 1;
    for (int i = 0; i < NUM_OF_BUILTIN_COMMANDS; i++) {
        printf("%s - %s\n", commands[i].name, commands[i].help);
    }
    fflush(stdout);
    return 0;
}

int prin() {
    for (int i = 1; i < token_count; i++) {
        if (i == token_count - 1) printf("%s", tokens[i]);
        else printf("%s ", tokens[i]);
        fflush(stdout);
    }
    return 0;
}

int echo() {
    for (int i = 1; i < token_count; i++) {
        if (i == token_count - 1)
            printf("%s", tokens[i]);
        else 
            printf("%s ", tokens[i]);
        fflush(stdout);
    }
    printf("\n");
    fflush(stdout);
    return 0;
}

int len() {
    int l = 0;
    for (int i = 1; i < token_count; i++) {
        l += strlen(tokens[i]);
    }
    printf("%d\n", l);
    fflush(stdout);
    return 0;
}

int sum() {
    int s = 0;
    for (int i = 1; i < token_count; i++) {
        int plus = atoi(tokens[i]);
        if (plus == 0) return 1;
        s += plus;
    }
    printf("%d\n", s);
    fflush(stdout);
    return 0;
}

int calc() {
    if (token_count != 4) return 1;
    int first = atoi(tokens[1]);
    int second = atoi(tokens[3]);
    if (first == 0 || second == 0) return 1;
    if (strcmp(tokens[2], "+") == 0) {
        printf("%d\n", first + second);
        return 0;
    }
    if (strcmp(tokens[2], "-") == 0) {
        printf("%d\n", first - second);
        return 0;
    }
    if (strcmp(tokens[2], "*") == 0) {
        printf("%d\n", first * second);
        return 0;
    }
    if (strcmp(tokens[2], "/") == 0) {
        printf("%d\n", first / second);
        return 0;
    }
    if (strcmp(tokens[2], "%") == 0) {
        printf("%d\n", first % second);
        return 0;
    }
    fflush(stdout);
    return 1;
}

int basename() {
    if (token_count != 2) return 1;
    char* last = strrchr(tokens[1], '/');
    if (last != NULL) {
        last++;
        printf("%s\n", last);
    } else {
        printf("%s\n", tokens[1]);
    }
    fflush(stdout);
    return 0;
}

int dirname() {
    if (token_count != 2) return 1;
    char cp[strlen(tokens[1]) + 1];
    strcpy(cp, tokens[1]);
    char* last = strrchr(cp, '/');
    if (last != NULL) {
        *last = '\0';
        if (strlen(cp) == 0) printf("/\n");
        else printf("%s\n", cp);
    } else {
        printf(".\n");
    }
    fflush(stdout);
    return 0;
}

int dirch() {
    if (token_count > 2) return 1;
    if (token_count == 1) {
        if (chdir("/") != 0) {
            int exi = errno;
            perror("dirch");
            fflush(stderr);
            return exi;
        }
    } else {
        if (chdir(tokens[1]) != 0) {
            int exi = errno;
            perror("dirch");
            fflush(stderr);
            return exi;
        }
    }
    return 0;
}

int dirwd() {
    if (token_count > 2) return 1;
    char cwd[MAX_PATH_LENGTH];
    getcwd(cwd, MAX_PATH_LENGTH);
    if (token_count == 2 && strcmp(tokens[1], "base") == 0 || token_count == 1) {
        char* last = strrchr(cwd, '/');
        last++;
        if (strlen(last) == 0) printf("/\n");
        else printf("%s\n", last);
    } else if (token_count == 2 && strcmp(tokens[1], "full") == 0) {
        printf("%s\n", cwd);
    } else {
        return 1;
    }
    fflush(stdout);
    return 0;
}

int dirmk() {
    if (token_count != 2) return 1;
    if (mkdir(tokens[1], 0755) == -1) {
        int exi = errno;
        perror("dirmk");
        fflush(stderr);
        return exi;
    }
    return 0;
}

int dirrm() {
    if (token_count != 2) return 1;
    if (rmdir(tokens[1]) == -1) {
        int exi = errno;
        perror("dirrm");
        fflush(stderr);
        return exi;
    }
    return 0;
}

int dirls() {
    if (token_count > 2) return 1;
    char* name = token_count == 1 ? "." : tokens[1];
    DIR* dir = opendir(name);
    int exi = errno;
    if (dir == NULL) {
        perror("dirls");
        fflush(stderr);
        return exi;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        printf("%s  ", entry -> d_name);
        fflush(stdout);
    }
    printf("\n");
    fflush(stdout);
    closedir(dir);
    return 0;
}

int ren() {
    if (token_count != 3) return 1;
    if (rename(tokens[1], tokens[2]) != 0) {
        int exi = errno;
        perror("rename");
        fflush(stderr);
        return exi;
    }
    return 0;
}

int unl() {
    if (token_count != 2) return 1;
    if (unlink(tokens[1]) != 0) {
        int exi = errno;
        perror("unlink");
        fflush(stderr);
        return exi;
    }
    return 0;
}

int rem() {
    if (token_count != 2) return 1;
    if (remove(tokens[1]) == -1) {
        int exi = errno;
        perror("remove");
        fflush(stderr);
        return exi;
    }
    return 0;
}

int linkh() {
    if (token_count != 3) return 1;
    if (link(tokens[1], tokens[2]) != 0) {
        int exi = errno;
        perror("linkhard");
        fflush(stderr);
        return exi;
    }
    return 0;
}

int links() {
    if (token_count != 3) return 1;
    if (symlink(tokens[1], tokens[2]) != 0) {
        int exi = errno;
        perror("linksoft");
        fflush(stderr);
        return exi;
    }
    return 0;
}

int linkr() {
    if (token_count != 2) return 1;
    char buff[BUFSIZE];
    int l = readlink(tokens[1], buff, BUFSIZE - 1);
    int exi = errno;
    if (l == -1) {
        perror("linkread");
        fflush(stderr);
        return exi;
    }
    buff[l] = '\0';
    printf("%s\n", buff);
    fflush(stdout);
    return 0;
}

int linkl() {
    if (token_count != 2) return 1;
    DIR* dir = opendir(".");
    int exi = errno;
    if (dir == NULL) {
        perror("linklist");
        fflush(stderr);
        return exi;
    }
    struct dirent *entry;
    struct stat st;
    struct stat st2;
    if (stat(tokens[1], &st) != 0) {
        exi = errno;
        perror("linklist");
        fflush(stderr);
        closedir(dir);
        return exi;
    }
    while ((entry = readdir(dir)) != NULL) {
        if (stat(entry -> d_name, &st2) == 0) {
            if (st.st_ino == st2.st_ino) {
                printf("%s  ", entry -> d_name);
                fflush(stdout);
            }
        }
    }
    printf("\n");
    fflush(stdout);
    closedir(dir);
    return 0;
}

int checkFirstArgv(char *a, int* in) {
    if (strcmp(a, "-") == 0) {
        *in = STDIN_FILENO;
    } else {
        *in = open(a, O_RDONLY);
        int exi = errno;
        if (*in == -1) {
            perror("cpcat");
            fflush(stderr);
            return exi;
        }
    }
    return 0;
}

int checkSecondArgv(char *a, int *out) {
    if (strcmp(a, "-") == 0) {
        *out = STDOUT_FILENO;
    } else {
        *out = open(a, O_WRONLY | O_CREAT, 0766);
        int exi = errno;
        if (*out == -1) {
            perror("cpcat");
            fflush(stderr);
            return exi;
        }
    }
    return 0;
}

int copyFromTo(int in, int out) {
    char buff[BUFSIZE];
    int r = 0, w = 0;
    while ((r = read(in, buff, BUFSIZE)) > 0) {
        w = write(out, buff, r);
        int exi = errno;
        if (w != r) {
            perror("cpcat");
            fflush(stderr);
            return exi;
        }
    }
    int exi = errno;
    if (r == -1) {
        perror("cpcat");
        fflush(stderr);
        return exi;
    }
    return 0;
}

int cpcat() {
    int in = 0, out = 0;
    if (token_count == 1) {
        in = STDIN_FILENO;
        out = STDOUT_FILENO;
    } else if (token_count == 2) {
        int exi = checkFirstArgv(tokens[1], &in);
        if (exi != 0) return exi;
        out = STDOUT_FILENO;
    } else if (token_count == 3) {
        int exi1 = checkFirstArgv(tokens[1], &in);
        int exi2 = checkSecondArgv(tokens[2], &out);
        if (exi1 != 0) return exi1;
        if (exi2 != 0) return exi2;
    } else {
        return 1;
    }
    int exi = copyFromTo(in, out);
    if (exi != 0) return exi;
    fflush(stdout);
    if (in != STDIN_FILENO) close(in);
    if (out != STDOUT_FILENO) close(out);
    return 0;
}

int pid() {
    if (token_count > 1) return 1;
    printf("%d\n", getpid());
    fflush(stdout);
    return 0;
}

int ppid() {
    if (token_count > 1) return 1;
    printf("%d\n", getppid());
    fflush(stdout);
    return 0;
}

int uid() {
    if (token_count > 1) return 1;
    printf("%d\n", getuid());
    fflush(stdout);
    return 0;
}

int euid() {
    if (token_count > 1) return 1;
    printf("%d\n", geteuid());
    fflush(stdout);
    return 0;
}

int gid() {
    if (token_count > 1) return 1;
    printf("%d\n", getgid());
    fflush(stdout);
    return 0;
}

int egid() {
    if (token_count > 1) return 1;
    printf("%d\n", getegid());
    fflush(stdout);
    return 0;
}

int sysinfo() {
    if (token_count > 1) return 1;
    struct utsname name;
    if (uname(&name) == -1) {
        int ex = errno;
        perror("sysinfo");
        fflush(stderr);
        return ex;
    }
    printf("Sysname: %s\n", name.sysname);
    printf("Nodename: %s\n", name.nodename);
    printf("Release: %s\n", name.release);
    printf("Version: %s\n", name.version);
    printf("Machine: %s\n", name.machine);
    fflush(stdout);
    return 0;
}

int proc() {
    if (token_count > 2) return 1;
    if (token_count == 1) {
        printf("%s\n", procfsPath);
        fflush(stdout);
    } else {
        if (access(tokens[1], F_OK | R_OK) != -1) {
            strcpy(procfsPath, tokens[1]);
        } else return 1;
    }
    return 0;
}

int pids() {
    if (token_count > 1) return 1;
    DIR* dir = opendir(procfsPath);
    int exi = errno;
    if (dir == NULL) {
        perror("pids");
        fflush(stderr);
        return exi;
    }
    struct dirent *entry;
    int* tableOfPids = (int*) malloc(sizeof(int) * MAX_NUM_OF_PROCESES);
    int i = 0;
    while ((entry = readdir(dir)) != NULL) {
        int digit = 1;
        char *na = entry -> d_name;
        while (*na) {
            if (!isdigit(*na)) {
                digit = 0;
                break;
            }
            na++;
        }
        if (digit) {
            int p = atoi(entry -> d_name);
            tableOfPids[i] = p;
            i++;
        }
    }
    qsort(tableOfPids, i, sizeof(int), compare);
    for (int j = 0; j < i; j++) {
        printf("%d\n", tableOfPids[j]);
    }
    fflush(stdout);
    closedir(dir);
    free(tableOfPids);
    return 0;
}

int compare(const void *a, const void *b) {
    int a1 = *((int*)a);
    int b1 = *((int*)b);

    if (a1 < b1) return -1;
    else if (a1 > b1) return 1;
    else return 0;
}

int pinfo() {
    if (token_count > 1) return 1;
    DIR* dir = opendir(procfsPath);
    int exi = errno;
    if (dir == NULL) {
        perror("pids");
        fflush(stderr);
        return exi;
    }
    struct dirent *entry;
    Pinfo* tab = (Pinfo*) malloc(sizeof(Pinfo) * MAX_NUM_OF_PROCESES);
    int  i = 0;
    while ((entry = readdir(dir)) != NULL) {
        int digit = 1;
        char *na = entry -> d_name;
        while (*na) {
            if (!isdigit(*na)) {
                digit = 0;
                break;
            }
            na++;
        }
        if (digit) {
            int len1 = strlen(procfsPath);
            int len2 = strlen(entry -> d_name);
            int len3 = strlen("/stat");
            char* na = (char*) malloc(sizeof(char) * (len1 + len2 + len3 + 1 + 1));
            strcpy(na, procfsPath);
            strcat(na, "/");
            strcat(na, entry -> d_name);
            strcat(na, "/stat");
            int file = open(na, O_RDONLY);
            int ex = errno;
            if (file == -1) {
                free(na);
                free(tab);
                close(file);
                perror("pinfo");
                fflush(stderr);
                return ex;
            }
            Pinfo pi;
            char buf[1024];
            int len = read(file, buf, sizeof(buf) - 1);
            ex = errno;
            if (len == -1) {
                perror("pinfo");
                fflush(stderr);
                close(file);
                free(na);
                free(tab);
                return ex;
            }
            buf[len] = '\0';
            sscanf(buf, "%d %s %c %d", &pi.p, pi.ime, &pi.s, &pi.pp);
            pi.ime[strlen(pi.ime) - 1] = '\0';
            tab[i] = pi;
            i++;
            free(na);
            close(file);
        }
    }
    qsort(tab, i, sizeof(Pinfo), compare2);
    printf("%5s %5s %6s %s\n", "PID", "PPID", "STANJE", "IME");
    for (int j = 0; j < i; j++) {
        printf("%5d %5d %6c %s\n", tab[j].p, tab[j].pp, tab[j].s, &tab[j].ime[1]);
    }
    free(tab);
    fflush(stdout);
    closedir(dir);
    return 0;
}

int compare2(const void *a, const void *b) {
    Pinfo *a1 = (Pinfo*)a;
    Pinfo *b1 = (Pinfo*)b;

    if (a1 -> p < b1 -> p) return -1;
    else if (a1 -> p > b1 -> p) return 1;
    else return 0;
}

int waitone() {
    if (token_count > 2) return 1;
    int s;
    if (token_count == 2) {
        if (waitpid(atoi(tokens[1]), &s, 0) == -1) {
            int ex = errno;
            perror("waitpid");
            fflush(stderr);
            return ex;
        } else {
            if (WIFEXITED(s)) {
                return WEXITSTATUS(s);
            }
        }
    } else {
        if (wait(&s) != -1) {
            if (WIFEXITED(s)) {
                return WEXITSTATUS(s);
            }
        }
    }
    return 0; 
}

int waitall() {
    if (token_count > 1) return 1;
    while (wait(NULL) != -1) {}
    return 0;
}

int pipes() {
    if (token_count < 3) return 1;
    int fd1[2];
    int fd2[2];
    int i;
    for (i = 1; i < token_count - 1; i++) {
        fd1[0] = fd2[0];
        fd1[1] = fd2[1];
        pipe(fd2);
        if (i == 1) {
            int ex = execute_child(tokens[i], NULL, fd2);
            if (ex != 0) return ex;
        } else {
            int ex = execute_child(tokens[i], fd1, fd2);
            if (ex != 0) return ex;
        }
        if (i > 1) {
            close(fd1[0]);
            close(fd1[1]);
        }
    }
    fd1[0] = fd2[0];
    fd1[1] = fd2[1];
    int ex = execute_child(tokens[i], fd1, NULL);
    if (ex != 0) return ex;
    close(fd1[0]);
    close(fd1[1]);
    for (int i = 1; i < token_count; i++) {
        wait(NULL);
    }
    fflush(stdout);
    return 0;
}

int execute_child(char* stopnja, int fd1[2], int fd2[2]) {
    fflush(stdin);
    int pid = fork();
    int ex = errno;
    if (pid < 0) {
        perror("fork");
        return ex;     
    } else if (pid == 0) {
        token_count = tokenize(stopnja);
        tokens[token_count] = NULL;
        if (fd1 != NULL) {
            dup2(fd1[0], 0);
            close(fd1[0]);
            close(fd1[1]);
        }
        if (fd2 != NULL) {
            dup2(fd2[1], 1);
            close(fd2[0]);
            close(fd2[1]);
        }
        int ix = find_builtin(tokens[0]);
        if (ix != -1) {
            ex = commands[ix].fun_pointer();
            exit(ex);
        } else {
            execvp(tokens[0], tokens);
            ex = errno;
            perror("execvp");
            exit(ex);
        }
    }
    return 0;
}