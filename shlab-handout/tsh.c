/* 
 * tsh - A tiny shell program with job control
 * 
 * <Put your name and login ID here>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/* Misc manifest constants */
#define MAXLINE    1024   /* max line size */
#define MAXARGS     128   /* max args on a command line */
#define MAXJOBS      16   /* max jobs at any point in time */
#define MAXJID    1<<16   /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/* 
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Global variables */
extern char **environ;      /* defined in libc */
char prompt[] = "tsh> ";    /* command line prompt (DO NOT CHANGE) */
int verbose = 0;            /* if true, print additional output */
int nextjid = 1;            /* next job ID to allocate */
char sbuf[MAXLINE];         /* for composing sprintf messages */

struct job_t {              /* The job struct */
    pid_t pid;              /* job PID */
    int jid;                /* job ID [1, 2, ...] */
    int state;              /* UNDEF, BG, FG, or ST */
    char cmdline[MAXLINE];  /* command line */
};
struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */


/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv); 
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs); 
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid); 
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid); 
int pid2jid(pid_t pid); 
void listjobs(struct job_t *jobs);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

/*
 * main - The shell's main routine 
 */
int main(int argc, char **argv) 
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2); //关闭标准错误，重定向到标准输出

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF) { //命令行参数解析
        switch (c) {
        case 'h':             /* print help message */
            usage();
	    break;
        case 'v':             /* emit additional diagnostic info */
            verbose = 1;
	    break;
        case 'p':             /* don't print a prompt */
            emit_prompt = 0;  /* handy for automatic testing */
	    break;
	default:
            usage();
	}
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT,  sigint_handler);   /* ctrl-c SIGINT 来自键盘的中断*/
    Signal(SIGTSTP, sigtstp_handler);  /* ctrl-z 来之终端的停止信号*/
    Signal(SIGCHLD, sigchld_handler);  /* Terminated or stopped child 一个子进程的停止或者终止*/

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler);  //来自键盘的退出

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1) {

	/* Read command line */
	if (emit_prompt) {
	    printf("%s", prompt); //打印shell标志
	    fflush(stdout);
	}
	if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
	    app_error("fgets error");
	if (feof(stdin)) { /* End of file (ctrl-d) feof检测文件是否读完，对标准输入流操作只有当ctrl-d的时候会返回流结束信号*/
	    fflush(stdout);
	    exit(0);
	}

	/* Evaluate the command line */
	eval(cmdline);
	fflush(stdout);
	fflush(stdout);
    } 

    exit(0); /* control never reaches here */
}
  
/* 
 * eval - Evaluate the command line that the user has just typed in
 * 
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
*/
void eval(char *cmdline) 
{
    char **argv[MAXARGS];
    char buf[MAXLINE];
    int bg;
    int state;
    //sigset_t mask_all,mask_one,prev;  //定义信号集合
    /*
    typedef struct {
                       unsigned long sig[_NSIG_WORDS]；
                       } sigset_t
    */
    strcpy(buf,cmdline);
    bg = parseline(buf,argv);
    if(argv[0]==NULL)
    {
        return;
    }
    if(!builtin_cmd(argv)) //判断是否是内置命令，如果是的直接执行，非内置命令则利用if内code
    {
        sigfillset(&mask_all);//sigfillset()用来将参数set 信号集初始化, 然后把所有的信号加入到此信号集里.
        sigemptyset(&mask_one); //将所有信号集合初始化为空。
        sigaddset(&mask_one,SIGCHLD);//该函数的作用是把信号SIGCHLD添加到信号集mask_one中，成功时返回0，失败时返回-1。
        sigprocmask(SIG_BLOCK,&mask_one,&prev);//该进程新的信号屏蔽字是其当前信号屏蔽字和mask_one指向信号集的并集。mask_one包含了我们希望阻塞的附加信号
        if((pid=fork())==0)
        {
            //子进程继承父进程的阻塞向量，也要解除阻塞。
            sigprocmask(SIG_SETMASK,&prev,NULL);//该进程新的信号屏蔽字将被prev指向的信号集的值代替，这其实就是删除了SIGCHLD信号的阻塞
            //改进程组与自己的PID相同。形成一个作业区。
            if(setpgid(0,0)<0)
            {
                app_error("SETPGID ERROR");
            }
            if(execve(argv[0],argv,environ)<0)
            {
                app_error("command not found");
            }

        }
        else
        {
            state = bg?BG:FG; // 前后台程序判断
            sigprocmask(SIG_BLOCK,&mask_all,NULL);
            addjob(jobs,pid,state,cmdline);
            sigprocmask(SIG_SETMASK,&prev,NULL);
        }
        if(!bg)
        {
            waitfg(pid); //前台进程等待子进程结束
        }
        else
        {
            printf("[%d] (%d) %s",pid2jid(pid),pid,cmdline);//后台进程执行打印，然后后台运行。
        }
    }
    /*
    if(bg) //这里实现了，前台进程的回收，但是后台进程如何回收呢？
    {
        int status;
        if(waitpid(pid,&status,0)<0)
        {
            app_error("waitpid error");
        }
        else
        {
            printf("%d  %s",pid,command);
        }
    }
    return;
}

/* 
 * parseline - Parse the command line and build the argv array.
 * 
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a BG job, false if
 * the user has requested a FG job.  
 */
int parseline(const char *cmdline, char **argv) //shell 参数用''包裹，或者' '（空格）相隔。返回对于BG job 和 FG job的判断
{
    static char array[MAXLINE]; /* holds local copy of command line */
    char *buf = array;          /* ptr that traverses command line */
    char *delim;                /* points to first space delimiter */
    int argc;                   /* number of args */
    int bg;                     /* background job? */

    strcpy(buf, cmdline);
    buf[strlen(buf)-1] = ' ';  /* replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* ignore leading spaces */
	   buf++;
    /* Build the argv list */
    argc = 0;
    if (*buf == '\'') {
	buf++;
	delim = strchr(buf, '\''); //返回指向buf里面第一个'字符的指针
    }
    else {
	delim = strchr(buf, ' ');
    }

    while (delim) {
	argv[argc++] = buf;
	*delim = '\0';
	buf = delim + 1;
	while (*buf && (*buf == ' ')) /* ignore spaces */
	       buf++;

	if (*buf == '\'') {
	    buf++;
	    delim = strchr(buf, '\'');
	}
	else {
	    delim = strchr(buf, ' ');
	}
    }
    argv[argc] = NULL;
    
    if (argc == 0)  /* ignore blank line */
	return 1;

    /* should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0) { //命令以&结尾则后台执行。
	argv[--argc] = NULL;
    }
    return bg;
}

/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.  
 */
int builtin_cmd(char **argv) 
{
    if(!strcmp(argv[0],"quit"))
    {
        exit(0);
    }
    else if(!strcmp(argv[0],"jobs"))
    {
        listjobs(jobs); //打印所有的进程信息。
        return 1;
    }
    else if(!strcmp(argv[0]."bg")||!strcmp(argv[0],"fg"))
    {
        do_bgfg(argv);
        return 1;
    }
    if(!strcmp(argv[0],"&")) //单独的&不处理
    {
        return 1;
    }    
    return 0;     /* not a builtin command */
}

/* 
 * do_bgfg - Execute the builtin bg and fg commands
 */
void do_bgfg(char **argv) 
{
    if(argv[1]==NULL)
    {
        printf("%s command requires PID or %%jobid argument\n",argv[0]);
        return;
    }
    struct job_t* job;
    int id;


    if(sscanf(argv[1],"%%d",&id)>0)
    {
        job = getjobjid(jobs,id); //以jid为判断标准
        if(job==NULL)
        {
            printf("%%%d: No such job\n",id);
            return;
        }
    }
    else if (sscanf(argv[1],"%%d",&id)>0)
    {
        /* code */
        job = getjobpid(jobs,id); //以pid为判断标准
        if(job==NULL)
        {
            printf("(%d): No such process\n",id);
            return;
        }
    }
    if(!strcmp(argv[0],"bg")) //这里就还是控制前后台运行了。
    {
        kill(-(job->pid),SIGCONT);
        job->state = BG;
        printf("[%d] (%d) %s",job->pid,job->pid,job->cmdline);
    }
    else
    {
        kill(-(job->pid),SIGCONT);
        job->state = FG;
        waitfg(job->pid);
    }
    return;
}

/* 
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid)
{
    sigset_t mask_temp;
    sigemptyset(&mask_temp);
    while(fgpid(jobs)>0)
    {
        sigsuspend(&mask_temp);//相当于该job不阻塞任何信号。同时挂起进程，这样当子进程结束就可以接着执行了。
    }
    return;
}

/*****************
 * Signal handlers
 *****************/

/* 
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.  
 */
void sigchld_handler(int sig) 
{
    int olderrno = errno;
    pid_t pid;
    int status;
    sigset_t mask_all,prev;
    sigfillset(&mask_all);
    while((pid=waitpid(-1,&status,WNOHANG | WUNTRACED))>0)
    {
        if(WIFEXITED(status))
        {
            sigprocmask(SIG_BLOCK,&mask_all,&prev);
            deletejob(jobs,pid);
            sigprocmask(SIG_SETMASK,&prev,NULL);
        }
        else if(WIFSIGNALED(status))
        {
            struct job_t * job = getjobpid(jobs,pid);
            sigprocmask(SIG_BLOCK,&mask_all,&prev);
            printf("Job [%d] (%d) terminate by signal %d\n",job->jid,job->pid,WTERMSIG(status));
            deletejob(jobs,pid);
            sigprocmask(SIG_SETMASK,&prev,NULL);
        }
        else
        {
            struct job_t * job = getjobpid(jobs,pid);
            sigprocmask(SIG_BLOCK,&mask_all,&prev);
            printf("Job [%d] (%d) stopped by signal %d\n",job->jid,job->pid,WTERMSIG(status));
            job->status = ST;
            sigprocmask(SIG_SETMASK,&prev,NULL);
        }
    }
    errno = olderrno;
    return;
}

/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */
void sigint_handler(int sig) 
{
    int olderrno = errno;
    pid_t pid = fgpid(jobs);
    if(pid!=0)
    {
        kill(-pid,sig);
    }
    errno = olderrno;
    return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.  
 */
void sigtstp_handler(int sig) 
{
    int olderrno = errno;
    pid_t pid = fgpid(jobs);
    if(pid!=0)
    {
        kill(-pid,sig);
    }
    errno = olderrno;
    return;
}

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job) {
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	clearjob(&jobs[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *jobs) 
{
    int i, max=0;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid > max)
	    max = jobs[i].jid;
    return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline) 
{
    int i;
    
    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == 0) {
	    jobs[i].pid = pid;
	    jobs[i].state = state;
	    jobs[i].jid = nextjid++;
	    if (nextjid > MAXJOBS)
		nextjid = 1;
	    strcpy(jobs[i].cmdline, cmdline);
  	    if(verbose){
	        printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
            }
            return 1;
	}
    }
    printf("Tried to create too many jobs\n");
    return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *jobs, pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == pid) {
	    clearjob(&jobs[i]);
	    nextjid = maxjid(jobs)+1;
	    return 1;
	}
    }
    return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
    	if (jobs[i].state == FG)
    	    return jobs[i].pid;
    return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t *getjobpid(struct job_t *jobs, pid_t pid) {
    int i;

    if (pid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid)
	    return &jobs[i];
    return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *jobs, int jid) 
{
    int i;

    if (jid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid == jid)
	    return &jobs[i];
    return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid) {
            return jobs[i].jid;
        }
    return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *jobs) 
{
    int i;
    
    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid != 0) {
	    printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
	    switch (jobs[i].state) {
		case BG: 
		    printf("Running ");
		    break;
		case FG: 
		    printf("Foreground ");
		    break;
		case ST: 
		    printf("Stopped ");
		    break;
	    default:
		    printf("listjobs: Internal error: job[%d].state=%d ", 
			   i, jobs[i].state);
	    }
	    printf("%s", jobs[i].cmdline);
	}
    }
}
/******************************
 * end job list helper routines
 ******************************/


/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void) 
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
    fprintf(stdout, "%s\n", msg);
    exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler) 
{
    struct sigaction action, old_action;

    action.sa_handler = handler;  
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
	unix_error("Signal error");
    return (old_action.sa_handler);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) 
{
    printf("Terminating after receipt of SIGQUIT signal\n");
    exit(1);
}



