        #include <stdlib.h>
        #include <errno.h>
        #include <stdio.h>
        #include <wait.h>
        #include "jobber.h"
        #include <string.h>
        #include "task.h"
        #include "hw4.h"
        /*
         * "Jobber" job spooler.
         */

int main(int argc, char *argv[])
{
    jobs_init();
    int exitWhile = 1;
    while(exitWhile)
    {

        char *readValue = sf_readline("jobber> ");
        if(*readValue == '\0')
            goto back;

        char temp_1 [20] ;
        int l=0;

        while(*readValue!= ' ' )
        {
            if(*readValue == '\0')
            {
                break;
            }
            temp_1[l] = *readValue;
            readValue++;
            l++;

        }
        char *num_1 = (readValue);
        int num = atoi(num_1);

        temp_1[l] = '\0';
        readValue = readValue - l;
         if(!strcmp(temp_1, "jobber"))
         {
            exit(EXIT_SUCCESS);
         }

        if(!strcmp(temp_1, "spool"))
        {
            readValue = readValue + 7;
            char *temp = readValue;
            char *qout = "\'";
            while(strcmp(readValue, qout))
            {
                readValue++;
            }
            *readValue = '\0';
            readValue = temp;
            int returnValue = job_create(readValue);
            if(returnValue == -1)
            {
                printf("Error in creating job.");
                fflush(stdout);
            }

        }
        else if(!strcmp(temp_1, "quit"))    //quit
        {
            jobs_fini();
            exitWhile = 0;
        }
       else if(!strcmp(temp_1, "enable"))     //enable
       {
        jobs_set_enabled(1);
    }
        else if(!strcmp(temp_1, "jobs"))   //jobs
        {
            print_jobs();
        }
        else if(!strcmp(temp_1, "disable"))    //disable
        {
            jobs_set_enabled(0);
        }
        else if(!strcmp(temp_1, "pause"))    //pause
        {
            // requires JOBID as argument;
            job_pause(num);
            //abort();
        }
        else if(!strcmp(temp_1, "resume"))    //resume
        {
            // requires JOBID as argument;
            job_resume(num);
            //abort();
        }
        else if(!strcmp(temp_1, "cancel"))    //cancel
        {
             // requires JOBID as argument;
            job_cancel(num);
            //abort();
        }
        else if(!strcmp(temp_1, "expunge"))    //expunge
        {
             // requires JOBID as argument;

            job_expunge(num);
            //abort();
        }
        else if(!strcmp(temp_1, "status"))    //status
        {
             // requires JOBID as argument;
            // not really sure rn
            job_get_status(num);

        }
        else if(!strcmp(temp_1, "help"))    //help
        {

            printHelp();
        }
        else {
            printf("Unrecognized command: %s\n", temp_1);
        }

    back:
    fflush(stdout);
        //}

    }


    exit(EXIT_SUCCESS);
}
