/* Mohammed Ahmed 
*  Student Number: 214396428
*  Date: Wednesday July 14,2021
*
* The program is a probablistic simulation of a client server system. 
* n amount of clients and servers are created. The client generates random amount of jobs and place
* it on a global queue. Server services that job at random times given if they're busy or not. This happens for
* n anount of intervals/ticks and each time the client and server are woken up to simulatie work done during this interval.
*/

#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

#include "queue.h"
#include "args.h"
#include "error.h"

int njobs,                                                        /* number of jobs ever created */
    ttlserv,                                                      /* total service offered by servers */
    ttlqlen;                                                      /* the total qlength */
int nblocked, nclientblocked; /* The number of threads blocked */ /*number of clent waiting for a job to be serviced*/
int nthreads;

/***********************************************************************
                           helper function
************************************************************************/

bool condition()
{
  bool result = (nblocked + nclientblocked) == ((nthreads)-1);
  return result; /* returns whether thread is the last to block */
}

/***********************************************************************
                           r a n d 0 _ 1
************************************************************************/
double rand0_1(unsigned int *seedp)
{
  double f;
  /* We use the re-entrant version of rand */
  f = (double)rand_r(seedp);
  return f / (double)RAND_MAX;
}

/***********************************************************************
                             C L I E N T
************************************************************************/

void *client(void *vptr)
{

  unsigned int seed;
  int lam;
  int pthrerr;
  struct thread_arg *ptr;

  ptr = (struct thread_arg *)vptr;

  while (1)
  {

    if (rand0_1(&ptr->seed) < ptr->lam) /*checks if a random number is less than lam*/
    {
      push_q(ptr->q); /* push to stack */

      pthrerr = pthread_mutex_lock(ptr->statex); /* Mutex Lock and catch error */
      if (pthrerr != 0)
        fatalerr("pthread lock", pthrerr, "failed");

      njobs++;

      pthrerr = pthread_mutex_unlock(ptr->statex); /* Mutex Unlock and catch error */
      if (pthrerr != 0)
        fatalerr("pthread unlock", pthrerr, "failed");

      pthrerr = pthread_mutex_lock(ptr->statex); /* Mutex Lock and catch error */
      if (pthrerr != 0)
        fatalerr("pthread lock", pthrerr, "failed");

      if (condition()) /* if last thread to block */
      {

        nblocked = 0; /* reset nblocked */

        pthrerr = pthread_cond_signal(ptr->clkblockcond); /* send signal to clock thread */
        if (pthrerr != 0)
          fatalerr("pthread cond_signal", pthrerr, "failed\n");
      }

      nclientblocked++;                            /* increment number of people in the client condition variable */
      pthrerr = pthread_mutex_unlock(ptr->statex); /* Mutex Unlock and catch error */
      if (pthrerr != 0)
        fatalerr("pthread unlock", pthrerr, "failed");

      pthrerr = pthread_cond_wait(ptr->sclientblockcond, ptr->blocktex); /* wait in the client condition variable */
      if (pthrerr != 0)
        fatalerr("pthread cond_wait", pthrerr, "failed\n");

      pthrerr = pthread_mutex_lock(ptr->statex); /* Mutex Lock and catch error */
      if (pthrerr != 0)
        fatalerr("pthread lock", pthrerr, "failed");

      nclientblocked--;                            /* decrement number block on client condition variable */
      pthrerr = pthread_mutex_unlock(ptr->statex); /* Mutex Unlock and catch error */
      if (pthrerr != 0)
        fatalerr("pthread unlock", pthrerr, "failed");
    }

    pthrerr = pthread_mutex_lock(ptr->statex); /* Mutex Lock and catch error */
    if (pthrerr != 0)
      fatalerr("pthread lock", pthrerr, "failed");

    if (condition()) /* if last thread to block */
    {
      nblocked = 0; /* reset nblocked */

      pthrerr = pthread_cond_signal(ptr->clkblockcond); /* send signal to clock thread */
      if (pthrerr != 0)
        fatalerr("pthread cond_signal", pthrerr, "failed\n");
    }

    else
    {

      nblocked++; /* increase number people blocked */
    }
    pthrerr = pthread_mutex_unlock(ptr->statex); /* Mutex Unlock and catch error */
    if (pthrerr != 0)
      fatalerr("pthread unlock", pthrerr, "failed");

    pthrerr = pthread_cond_wait(ptr->thrblockcond, ptr->blocktex); /* wait for clock signal in the thread condition variable */
    if (pthrerr != 0)
      fatalerr("pthread cond_wait", pthrerr, "failed\n");
  }
  return NULL;
}

/***********************************************************************
                             S E R V E R
************************************************************************/

void *server(void *vptr)
{

  int busy;
  int pthrerr;
  struct thread_arg *ptr;

  ptr = (struct thread_arg *)vptr;

  busy = 0;
  while (1)
  {

    if (busy == 0) /* if server is not busy */
    {

      if (size_q(ptr->q) != 0) /* if size of queue is not equal to 0 */
      {

        safepop_q(ptr->q); /* pop of the queue a job */

        busy = 1; /*make thread busy */
      }
    }
    else /* thread is busy */
    {
      if (rand0_1(&ptr->seed) < ptr->mu) /* check whether you can service the job */
      {

        busy = 0;                                             /* change to not busy */
        pthrerr = pthread_cond_signal(ptr->sclientblockcond); /* send a signal to clients blocked */
        if (pthrerr != 0)
          fatalerr("pthread cond_signal", pthrerr, "failed\n");

        pthrerr = pthread_mutex_lock(ptr->statex); /* Mutex Lock and catch error */
        if (pthrerr != 0)
          fatalerr("pthread lock", pthrerr, "failed");

        ttlserv++;

        pthrerr = pthread_mutex_unlock(ptr->statex); /* Mutex Unlock and catch error */
        if (pthrerr != 0)
          fatalerr("pthread unlock", pthrerr, "failed");
      }
    }
    pthrerr = pthread_mutex_lock(ptr->statex); /* Mutex Lock and catch error */
    if (pthrerr != 0)
      fatalerr("pthread lock", pthrerr, "failed");

    if (condition()) /* if last thread to block */
    {

      nblocked = 0; /* reset nblocked */

      pthrerr = pthread_cond_signal(ptr->clkblockcond); /* send signal to clock */
      if (pthrerr != 0)
        fatalerr("pthread cond_signal", pthrerr, "failed\n");
    }
    else
    {

      nblocked++; /* increase number of threads blocked */
    }
    pthrerr = pthread_mutex_unlock(ptr->statex); /* Mutex Unlock and catch error */
    if (pthrerr != 0)
      fatalerr("pthread unlock", pthrerr, "failed");

    pthrerr = pthread_cond_wait(ptr->thrblockcond, ptr->blocktex); /* wait for signal in clock the thread condition variable */
    if (pthrerr != 0)
      fatalerr("pthread cond_wait", pthrerr, "failed\n");
  }
  return NULL;
}

/***********************************************************************
                                C L K
************************************************************************/

void *clk(void *vptr)
{

  int tick;
  int pthrerr;
  struct thread_arg *ptr;
  ptr = (struct thread_arg *)vptr;

  int j = 0;
  while (j < ptr->nticks) /* for loop on the number of ticks */
  {

    pthrerr = pthread_cond_wait(ptr->clkblockcond, ptr->blocktex); /* wait for signal in the clock condition variable */
    if (pthrerr != 0)
      fatalerr("pthread cond_wait", pthrerr, "failed\n");

    pthrerr = pthread_mutex_lock(ptr->statex); /* Mutex Lock and catch error */
    if (pthrerr != 0)
      fatalerr("pthread lock", pthrerr, "failed");

    ttlqlen += size_q(ptr->q); /* increase ttlqlen */

    pthrerr = pthread_mutex_unlock(ptr->statex); /* Mutex Unlock and catch error */
    if (pthrerr != 0)
      fatalerr("pthread unlock", pthrerr, "failed");

    pthrerr = pthread_cond_broadcast(ptr->thrblockcond); /* broadcast to thread condition varible */
    if (pthrerr != 0)
      fatalerr("pthread cond_wait", pthrerr, "failed\n");

    j++;
  }

  printf("Average waiting time:    %f\n", (float)ttlqlen / (float)njobs);
  printf("Average turnaround time: %f\n", (float)ttlqlen / (float)njobs +
                                              (float)ttlserv / (float)njobs);
  printf("Average execution time:  %f\n", (float)ttlserv / (float)njobs);
  printf("Average queue length: %f\n", (float)ttlqlen / (float)ptr->nticks);
  printf("Average interarrival time time: %f\n", (float)ptr->nticks / (float)njobs);
  /* Here we die with mutex locked and everyone else asleep */

  pthrerr = pthread_cancel(pthread_self()); /* kill thread */
  if (pthrerr != 0)
    fatalerr("pthread cancel", pthrerr, "failed\n");
  exit(0);
}

int main(int argc, char **argv)
{

  int pthrerr, i;
  int nserver, nclient, nticks;
  float lam, mu;

  pthread_t server_tid, client_tid;
  pthread_cond_t sthrblockcond, sclkblockcond, sclientblockcond;
  pthread_mutex_t sblocktex, sstatex;
  struct thread_arg *allargs;
  pthread_t *alltids;
  queue_t *queue = mk_queue();

  /* initialize mutex and condtion variables */
  pthrerr = pthread_mutex_init(&sblocktex, NULL);
  if (pthrerr != 0)
    fatalerr("Mutex init", pthrerr, "failed\n");
  pthrerr = pthread_mutex_init(&sstatex, NULL);
  if (pthrerr != 0)
    fatalerr("Mutex init", pthrerr, "failed\n");
  pthrerr = pthread_cond_init(&sthrblockcond, NULL);
  if (pthrerr != 0)
    fatalerr("Condition init", pthrerr, "failed\n");
  pthrerr = pthread_cond_init(&sclkblockcond, NULL);
  if (pthrerr != 0)
    fatalerr("Condition init", pthrerr, "failed\n");
  pthrerr = pthread_cond_init(&sclientblockcond, NULL);
  if (pthrerr != 0)
    fatalerr("Condition init", pthrerr, "failed\n");

  ttlserv = 0;
  ttlqlen = 0;
  nblocked = 0;
  njobs = 0;

  nserver = 2;
  nclient = 2;
  lam = 0.005;
  mu = 0.01;
  nticks = 1000;

  i = 1;
  while (i < argc - 1)
  {
    if (strncmp("--lambda", argv[i], strlen(argv[i])) == 0)
      lam = atof(argv[++i]);
    else if (strncmp("--mu", argv[i], strlen(argv[i])) == 0)
      mu = atof(argv[++i]);
    else if (strncmp("--servers", argv[i], strlen(argv[i])) == 0)
      nserver = atoi(argv[++i]);
    else if (strncmp("--clients", argv[i], strlen(argv[i])) == 0)
      nclient = atoi(argv[++i]);
    else if (strncmp("--ticks", argv[i], strlen(argv[i])) == 0)
      nticks = atoi(argv[++i]);
    else
      fatalerr(argv[i], 0, "Invalid argument\n");
    i++;
  }
  if (i != argc)
    fatalerr(argv[0], 0, "Odd number of args\n");

  allargs = (struct thread_arg *)
      malloc((nserver + nclient + 1) * sizeof(struct thread_arg));
  if (allargs == NULL)
    fatalerr(argv[0], 0, "Out of memory\n");
  alltids = (pthread_t *)
      malloc((nserver + nclient) * sizeof(pthread_t));
  if (alltids == NULL)
    fatalerr(argv[0], 0, "Out of memory\n");

  /* initalize number of total threads */
  nthreads = nserver + nclient;

  /* initalize client and server threads and pass arguments needed for each thread */
  int y = 1;
  while (y <= nclient)
  {
    allargs[y].lam = lam;
    allargs[y].thrblockcond = &sthrblockcond;
    allargs[y].clkblockcond = &sclkblockcond;
    allargs[y].sclientblockcond = &sclientblockcond;
    allargs[y].blocktex = &sblocktex;
    allargs[y].statex = &sstatex;
    allargs[y].nserver = nserver;
    allargs[y].nclient = nclient;
    allargs[y].nticks = nticks;
    allargs[y].q = queue;

    allargs[y].seed = y + clock();
    pthrerr = pthread_create(&alltids[y], NULL, &client, (void *)&allargs[y]);
    if (pthrerr != 0)
      fatalerr("Create", pthrerr, "failed\n");
    y++;
  }
  int x = 1;
  while (x <= nserver)
  {
    allargs[x].mu = mu;
    allargs[x].thrblockcond = &sthrblockcond;
    allargs[x].clkblockcond = &sclkblockcond;
    allargs[x].sclientblockcond = &sclientblockcond;
    allargs[x].blocktex = &sblocktex;
    allargs[x].nticks = nticks;
    allargs[x].nserver = nserver;
    allargs[x].nclient = nclient;
    allargs[x].statex = &sstatex;
    allargs[x].seed = x + clock();
    allargs[x].q = queue;

    pthrerr = pthread_create(&alltids[x], NULL, &server, (void *)&allargs[x]);
    if (pthrerr != 0)
      fatalerr("Create", pthrerr, "failed\n");

    x++;
  }

  /* pass argument to clock thread */
  allargs[0].nticks = nticks;
  allargs[0].clkblockcond = &sclkblockcond;
  allargs[0].blocktex = &sblocktex;
  allargs[0].statex = &sstatex;
  allargs[0].q = queue;
  allargs[0].thrblockcond = &sthrblockcond;

  /* call clock thread */
  clk(&allargs[0]);

  /* destroy condition and mutex variable */
  pthread_cond_destroy(&sclientblockcond);
  pthread_cond_destroy(&sthrblockcond);
  pthread_cond_destroy(&sclkblockcond);
  pthread_mutex_destroy(&sblocktex);
  pthread_mutex_destroy(&sstatex);

  exit(-1);
}
