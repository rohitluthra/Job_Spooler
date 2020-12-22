/*
 * Job manager for "jobber".
 */

#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include "jobber.h"
#include "task.h"
#include "hw4.h"

#define zindex1 1   // zindex = 1
#define zindex0 2   // zindex = 0
#define zindexm1 3  // technically it is -1

void myownsigchldHandler(int signalIncoming);
void myownsigtstpHandler(int signalIncoming);
void myownsigintHandler(int signalIncoming);
void myownsigquitHandler(int signalIncoming);
int enable(void);
void printprocesstracker(void);

void printMystructArray(void);
void printterminalError (char *error);
int getJobStatusOfCurrentJob();
void printcurrentjob(TASK *task);
// #define MAXLENGTHOFINPUT 1024

typedef struct job_struct{
    int jobID;
    JOB_STATUS jobState;
    pid_t processId;
    char *taskToPerform;
}job_struct;

typedef struct TRACKCHILDREN{
    pid_t child;
    struct TRACKCHILDREN* nextChild;
}TRACKCHILDREN;

typedef struct TRACKRUNNERANDCHILDREN{
    pid_t parent;
    TRACKCHILDREN *tc;
}TRACKRUNNERANDCHILDREN;

struct job_struct jobsArray[MAX_JOBS];

pid_t processidtracker[4] ;

volatile sig_atomic_t sigintvar = 0;
volatile sig_atomic_t sigtstipvar = 0;
volatile sig_atomic_t sigchldvar = 0;
volatile sig_atomic_t sigquitvar = 0;
int enabledd = 0;
int runnerCounter = 0;
int runnerCounterJobs =0;


int jobs_init(void) {

    signal(SIGINT, myownsigintHandler);
    signal(SIGTSTP, myownsigtstpHandler);
    signal(SIGCHLD, myownsigchldHandler);
    signal(SIGQUIT, myownsigquitHandler);

    for(int i=0; i<MAX_JOBS; i++)
    {
        struct job_struct *incomingJobs = malloc (sizeof(job_struct));
        incomingJobs->jobID = i;
        incomingJobs-> processId = 0;
        jobsArray[i] = *incomingJobs;

        free(incomingJobs);
    }
   // debug("ff");
    for(int i=0; i<4; i++)
    {

        processidtracker[i] = 0;
    }

    return 0;
}

int job_create(char *command)
{
    int i=0;

    while(jobsArray[i].jobState != NEW && i<MAX_JOBS)
    {
        i++;
    }

    if(i<=MAX_JOBS){
        jobsArray[i].jobState = WAITING;
        jobsArray[i].processId = 0;
        jobsArray[i].taskToPerform = command;
       // debug("\n\n from create: %s", command);
    }
    else
    {
        return -1;
    }
    printf("TASK: %s\n",job_get_taskspec(i));
    fflush(stdout);

    sf_job_create(i);
    sf_job_status_change(i, NEW, WAITING);

    if(enabledd)
    {
        enable();
    }

    return i;
}

int enable(void){
    pid_t pid;
    sigset_t set;

    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    sigprocmask(SIG_BLOCK, &set, NULL);

    for(int i=0; i < MAX_JOBS;i++)
    {
        if(runnerCounter<4)                                                 // if runnerprocess are not available then job wont start
        {
            if(jobsArray[i].jobState == WAITING)
            {
                pid = fork();                                               // This is our main Fork().

                if(pid < 0)
                {
                    sigprocmask(SIG_UNBLOCK, &set, NULL);
                    printterminalError("Error in Forking");
                }

                else if(pid == 0)
                {

                    sf_job_start(i, job_get_pgid(i));
                    sf_job_status_change(i, WAITING, RUNNING);

                    sigprocmask(SIG_UNBLOCK, &set, NULL);
                    setpgid(0,0);
                    char *temp = job_get_taskspec(i);
                    char **copyoftemp = &temp;
                    TASK *task = parse_task(copyoftemp);

                    PIPELINE_LIST *pipelinelist = task->pipelines;
                    while(pipelinelist!=NULL)
                    {
                        PIPELINE *pipeline = pipelinelist -> first;
                        //int count = 0 ;


                        pid_t childPID;
                        childPID = fork();

                        if(childPID < 0)
                        {
                           // sigprocmask(SIG_UNBLOCK, &set, NULL);
                            printterminalError("Error in Forking");
                        }
                        else if(childPID == 0)
                        {
                            COMMAND_LIST *commandlist = pipeline->commands;
                           // pid_t childProcessTracker[30];
                            int j =0;
                            while(commandlist!=NULL)
                            {
                                pid_t anotherchild;
                                anotherchild = fork();


                                if(anotherchild < 0)
                                {
                                    //sigprocmask(SIG_UNBLOCK, &set, NULL);
                                    printterminalError("Error in Forking");
                                }

                                else if(anotherchild == 0)
                                {
                                   // childProcessTracker[j] = getpid();
                                    j++;

                                    COMMAND *command = commandlist->first;
                                    WORD_LIST *wordlist = command -> words;
                                    char *args[1024];
                                    int j=0;

                                    while (wordlist != NULL)
                                    {
                                        args[j] = wordlist->first;
                                        fflush(stdout);
                                        wordlist = wordlist->rest;
                                        j++;
                                    }
                                    args[j] = NULL;

                                    if (execvp (args[0], args)<0)
                                    {
                                        printf("error in command: %s", args[0]);
                                        exit(0);
                                    }
                                    exit(0);
                                }
                                else
                                {
                                    int status;
                                    waitpid(anotherchild, &status,0);
                                }
                                commandlist = commandlist->rest;
                            }
                            exit(0);
                        }

                        else{

                           //  int status;
                           // // pid_t pp ;

                           //  for(int l= 0; l<j;l++)
                           //  {
                           //      printf("waiting for %d command to finish\n", l);
                           //      waitpid(childProcessTracker[l], &status,0);
                           //      printf("%d command finished\n", l);
                           //  }
                           //  if(WIFEXITED(status)){
                           //     sf_job_end(childPID, job_get_pgid(i), WEXITSTATUS(status));
                           //     sf_job_status_change(childPID, RUNNING, COMPLETED);
                           // }
                            int status;
                          //  printf("waiting for 2 command to finish\n");

                            pid_t radn= waitpid(childPID, &status, 0);
                          //  printf("2 command finished\n");

                            if ( radn ==-1)
                            {
                                printf("Waitpid Error: ");
                                abort();
                            }
                            else{
                                // if(WIFEXITED(status))
                                // {
                                //     printf("\nfrom pipeline\n");
                                //     sf_job_end(childPID, job_get_pgid(i), WEXITSTATUS(status));
                                //     sf_job_status_change(childPID, RUNNING, COMPLETED);
                                // }
                            }
                        }
                        pipelinelist = pipelinelist ->rest;
                    }
                    exit(0);
                }

                else {

                    int status;
                   // printf("waiting for 3 command to finish\n");
                    waitpid(pid, &status,0);
                  //  printf("3 command finished\n");

                    if (WIFEXITED(status))
                    {
                       // printf("\nfrom final\n");
                        sf_job_end(i, job_get_pgid(i), WEXITSTATUS(status));
                        sf_job_status_change(i, RUNNING, COMPLETED);
                       // free_task(task);
                        jobsArray[i].jobState = COMPLETED;
                        sigprocmask(SIG_UNBLOCK, &set, NULL);
                    }
                    else
                    {
                        printterminalError("Wait: error");
                    }

                }
            }
        }
    }
    //printprocesstracker();
    return 0;
}


void jobs_fini(void) {
    // TO BE IMPLEMENTED
    return;
}

int jobs_set_enabled(int val) {
    enabledd = val;
    if(enabledd)
        enable();
    return enabledd;
}

int jobs_get_enabled() {
    return enabledd;
}

int job_expunge(int jobid) {

    if(jobid < 0 && jobid>MAX_JOBS)
    {
        printf("Number not in range");
        return -1;
    }
    if(jobsArray[jobid].jobState  == COMPLETED || jobsArray[jobid].jobState  == ABORTED)
    {
        jobsArray[jobid].jobState = NEW;
        jobsArray[jobid].processId = 0;
        jobsArray[jobid].taskToPerform = "";
        sf_job_expunge(jobid);
        return 0;
    }
    return -1;
}

int job_cancel(int jobid) {
    if(jobid < 0 && jobid>MAX_JOBS)
    {
        printf("Number not in range");
        return -1;
    }
    if(jobsArray[jobid].jobState  == WAITING || jobsArray[jobid].jobState  == RUNNING|| jobsArray[jobid].jobState  == PAUSED)
    {
        jobsArray[jobid].jobState = CANCELED;
        return 0;
    }
    return -1;
}

int job_pause(int jobid) {
    if(jobid < 0 && jobid>MAX_JOBS)
    {
        printf("Number not in range");
        return -1;
    }
    if(jobsArray[jobid].jobState  == RUNNING)
    {
        jobsArray[jobid].jobState = PAUSED;
        sf_job_pause(jobid, job_get_pgid(jobid));
        return 0;
    }
    return -1;

}

int job_resume(int jobid) {
        if(jobid < 0 && jobid>MAX_JOBS)
    {
        printf("Number not in range");
        return -1;
    }
    if(jobsArray[jobid].jobState  == PAUSED)
    {
        jobsArray[jobid].jobState = RUNNING;
        sf_job_resume(jobid, job_get_pgid(jobid));
        return 0;
    }
    return -1;
}

int job_get_pgid(int jobid)
{
    if(jobid < 0 && jobid>MAX_JOBS)
    {
        return -1;
    }

     return getpgid(jobsArray[jobid].processId) ;


}

JOB_STATUS job_get_status(int jobid) {
    if(jobid < 0 && jobid>MAX_JOBS)
    {
        return -1;
    }

        char *value = "";
        if(jobsArray[jobid].jobState == 0)
        {
            value = "new";

        }
        else if(jobsArray[jobid].jobState == 1)
        {
            value = "waiting";
        }
        else if(jobsArray[jobid].jobState == 2)
        {
            value = "running";
        }
        else if(jobsArray[jobid].jobState == 3)
        {
            value = "paused";
        }
        else if(jobsArray[jobid].jobState == 4)
        {
            value = "canceled";
        }
        else if(jobsArray[jobid].jobState == 5)
        {
            value = "completed";
        }
        else if(jobsArray[jobid].jobState == 6)
        {
            value = "aborted";
        }

        printf("job %d [%s]: %s\n",jobid , value, jobsArray[jobid].taskToPerform );

    return jobsArray[jobid].jobState;
}

int job_get_result(int jobid) {
      if(jobid < 0 && jobid>MAX_JOBS)
    {
        printf("Number not in range");
        return -1;
    }
    if(jobsArray[jobid].jobState  == COMPLETED)
    {
        return  0;
    }
    return -1;
}

int job_was_canceled(int jobid) {
    if(jobid < 0 || jobid >MAX_JOBS)
    {
        return -1;
    }
    if(jobsArray[jobid].jobState  == ABORTED)
    {
        kill(-jobsArray[jobid].processId, 0);
        return 0;
    }
    return -1;
}

char *job_get_taskspec(int jobid) {
    if(jobid < 0 || jobid >MAX_JOBS)
    {
        return NULL;
    }
    return jobsArray[jobid].taskToPerform;
}
/*
Following are helper functions for my implementation of jobber.
*/
void printHelp(){
    printf("Available commands:\n");
    printf("help (0 args) Print this help message\n");
    printf("quit (0 args) Quit the program\n");
    printf("enable (0 args) Allow jobs to start\n");
    printf("disable (0 args) Prevent jobs from starting\n");
    printf("spool (1 args) Spool a new job\n");
    printf("pause (1 args) Pause a running job\n");
    printf("resume (1 args) Resume a paused job\n");
    printf("cancel (1 args) Cancel an unfinished job\n");
    printf("expunge (1 args) Expunge a finished job\n");
    printf("status (1 args) Print the status of a job \n");
    printf("jobs (0 args) Print the status of all jobs\n");
    // fflush(stdout);
}

void myownsigchldHandler(int signalIncoming)
{
    sigchldvar = 1;
    // int status;
    // pid_t ttape ;
    // while( (ttape = waitpid(-1, &status,WNOHANG|WUNTRACED))>0)
    // {
    //     //
    //     int m;
    //     for(m = 0; m < 4; m++)
    //     {
    //         if(processidtracker[m] == ttape)
    //         {
    //             for(int l=0;l<MAX_JOBS;l++)
    //             {
    //                 if(jobsArray[l].processId == processidtracker[m])
    //                 {
    //                     processidtracker[m]=0;
    //                     runnerCounter--;

    //                     if (WIFEXITED(status))
    //                     {
    //                         sf_job_end(l, job_get_pgid(l), WEXITSTATUS(status));
    //                         sf_job_status_change(l, RUNNING, COMPLETED);
    //                         jobsArray[l].jobState = COMPLETED;
    //                         //debug("runner counter: %d\n", runnerCounter);
    //                         return;
    //                     }
    //                     else
    //                     {
    //                         printterminalError("Wait: error");
    //                     }
    //                 }
    //             }

    //         }
    //     }
    // }

}
void myownsigtstpHandler(int signalIncoming)
{
    sigtstipvar = 1;
    for(int i=0; i<MAX_JOBS;i++)
    {
        if(jobsArray[i].jobState == RUNNING)
        {
            // kill first argument, may be negative
            kill(-jobsArray[i].processId, signalIncoming);
        }
    }
    exit(1);
}
void myownsigintHandler(int signalIncoming)
{
    sigintvar = 1;
    for(int i=0; i<MAX_JOBS;i++)
    {
        if(jobsArray[i].jobState == RUNNING)
        {
            kill(-jobsArray[i].processId, signalIncoming);
        }
    }
    exit(1);
}
void myownsigquitHandler(int signalIncoming)
{
    sigquitvar = 1;
    printf("Received SIGQUIT");
    fflush(stdout);
    exit(1);
}

void printMystructArray(void)
{
    for(int i=0; i<MAX_JOBS; i++)
    {
        debug("jobID: %d jobState: %d processId: %d", jobsArray[i].jobID,jobsArray[i].jobState,jobsArray[i].processId);
    }
}

void printterminalError (char *error)
{
    fprintf(stdout, "%s %s\n",error ,strerror(errno));
    fflush(stdout);
    exit(1);
}

void printinsiderror(char *error)
{
    fprintf(stdout, "%s\n", error);
    fflush(stdout);
    exit(1);
}

int getJobStatusOfCurrentJob ()
{
    int i=0;
    int worseCase = 0;
    while (i < MAX_JOBS)
    {
        if( jobsArray[i].jobState == NEW)
        {
            return jobsArray[i].processId;
        }
        i++;
    }
    return worseCase;

}

void print_jobs(){
    if(!enabledd)
    {
        printf("Starting jobs is disabled\n");

    }
    else{
        printf("Starting jobs is enabled\n");

    }
    fflush(stdout);
    for(int i=0; i<MAX_JOBS;i++)
    {
        char *value = "";
        if(jobsArray[i].jobState == 0)
        {
            value = "new";
            goto skip;
        }
        else if(jobsArray[i].jobState == 1)
        {
            value = "waiting";
        }
        else if(jobsArray[i].jobState == 2)
        {
            value = "running";
        }
        else if(jobsArray[i].jobState == 3)
        {
            value = "paused";
        }
        else if(jobsArray[i].jobState == 4)
        {
            value = "canceled";
        }
        else if(jobsArray[i].jobState == 5)
        {
            value = "completed";
        }
        else if(jobsArray[i].jobState == 6)
        {
            value = "aborted";
        }

        printf("job %d [%s]: %s\n", i, value, jobsArray[i].taskToPerform );
        skip:
        fflush(stdout);
    }

}
void printprocesstracker(void)
{

}

void printcurrentjob(TASK *task)
{
    // int wlnum = 1;
    // int plnum = 1;
    //   PIPELINE_LIST *pipelinelist = task->pipelines;              // This is pipelines PIPELINE_LIST
    //   while(pipelinelist!=NULL)
    //   {
    //    printf("\nPipeline: #%d ", plnum);
    //              PIPELINE *pipeline = pipelinelist -> first;                 // This is PIPELINE_LIST first pipeline
    //              COMMAND_LIST *commandlist = pipeline->commands;             // This is commandlist from pipeline.
    //              while(commandlist!=NULL)
    //              {
    //                 printf("\n\t Command list #%d \n\t", wlnum);
    //              COMMAND *command = commandlist->first;                      // This is first command in command list
    //              WORD_LIST *wordlist = command -> words;

    //              while (wordlist != NULL)
    //              {
    //                 printf("Word: %s ",wordlist->first);
    //                 fflush(stdout);
    //                 wordlist = wordlist->rest;
    //             }

    //             commandlist = commandlist->rest;
    //             wlnum++;
    //         }

    //         pipelinelist = pipelinelist ->rest;
    //         plnum++;
    //     }
}

