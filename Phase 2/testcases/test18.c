
/* checking for release: 3 instances of XXp2 receive messages from a zero-slot
 * mailbox, which causes them to block. XXp4 then releases the mailbox.
 * All processes are at the same priority.
 */

#include <stdio.h>
#include <strings.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>

int XXp2(char *);
int XXp3(char *);
int XXp4(char *);
char buf[256];

int mbox_id;

int start2(char *arg)
{
   int kid_status, kidpid, pausepid;

   printf("start2(): started\n");
   mbox_id  = MboxCreate(0, 50);
   printf("start2(): MboxCreate returned id = %d\n", mbox_id);

   kidpid   = fork1("XXp2a", XXp2, "XXp2a", 2 * USLOSS_MIN_STACK, 3);
   kidpid   = fork1("XXp2b", XXp2, "XXp2b", 2 * USLOSS_MIN_STACK, 3);
   kidpid   = fork1("XXp2c", XXp2, "XXp2c", 2 * USLOSS_MIN_STACK, 3);
   pausepid = fork1("XXp4",  XXp4, "XXp4",  2 * USLOSS_MIN_STACK, 3);
   kidpid = join(&kid_status);
   if (kidpid != pausepid)
      printf("\n***Test Failed*** -- join with pausepid failed!\n\n");

   kidpid   = fork1("XXp3",  XXp3, NULL,    2 * USLOSS_MIN_STACK, 2);

   kidpid = join(&kid_status);
   printf("start2(): joined with kid %d, status = %d\n", kidpid, kid_status);

   kidpid = join(&kid_status);
   printf("start2(): joined with kid %d, status = %d\n", kidpid, kid_status);

   kidpid = join(&kid_status);
   printf("start2(): joined with kid %d, status = %d\n", kidpid, kid_status);

   kidpid = join(&kid_status);
   printf("start2(): joined with kid %d, status = %d\n", kidpid, kid_status);

   quit(0);
   return 0; /* so gcc will not complain about its absence... */
} /* start2 */


int XXp2(char *arg)
{
   int result;
   char buffer[20];

   // initialize for strings
   buffer[0] = 0;

   result = MboxReceive(mbox_id, buffer, 20);
   printf("%s(): after recv of message '%s', result = %d\n",
          arg, buffer, result);

   if (result == -3)
      printf("%s(): zap'd by MboxReceive() call\n", arg);

   quit(-3);
   return 0;

} /* XXp2 */


int XXp3(char *arg)
{
   int result;

   printf("XXp3(): started\n");

   result = MboxRelease(mbox_id);

   printf("XXp3(): MboxRelease returned %d\n", result);

   quit(-4);
   return 0;
} /* XXp3 */


int XXp4(char *arg)
{

   printf("XXp4(): started and quitting\n");
   quit(-4);

   return 0;
}
