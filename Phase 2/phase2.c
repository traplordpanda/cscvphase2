/* ------------------------------------------------------------------------
   Kyle AuBuchon & Juan Gonzalez
   phase2.c
   Applied Technology
   College of Applied Science and Technology
   The University of Arizona
   CSCV 452

   ------------------------------------------------------------------------ */
#include <stdlib.h>
#include <phase1.h>
#include <phase2.h>
#include <usloss.h>

#include "message.h"

/* ------------------------- Prototypes ----------------------------------- */
int start1 (char *);
extern int start2 (char *);

void check_kernel_mode(char *message);
void debugConsole(char *message);
static void enableInterrupts();
static void disableInterrupts();

int 		check_io();

void		zeroMailbox(int mailbox_ID);
void		zeroSlot(int slot_ID);
void		zeroMboxSlot(int pid);


int			MboxRelease(int mailboxID);
int			MboxCondSend(int mailboxID, void *msg, int msg_Size);
int			MboxCondReceive(int mailboxID, void *msg, int max_msg_Size);

slot_ptr    createSlot(int index, int mbox_ID, void *msg, int msg_Size);
int			getSlot();
//int			addSlotToList(slot_ptr slot,  mailbox_ptr);

/* -------------------------- Globals ------------------------------------- */

int debugflag2 = 0;

/* the mail boxes */
mail_box MailBoxTable[MAXMBOX];


/* -------------------------- Functions -----------------------------------
  Below I have code provided to you that calls

  check_kernel_mode
  enableInterrupts
  disableInterupts
  
  These functions need to be redefined in this phase 2,because
  their phase 1 definitions are static 
  and are not supposed to be used outside of phase 1.  */

/* ------------------------------------------------------------------------
   Name - start1
   Purpose - Initializes mailboxes and interrupt vector.
             Start the phase2 test process.
   Parameters - one, default arg passed by fork1, not used here.
   Returns - one to indicate normal quit.
   Side Effects - lots since it initializes the phase2 data structures.
   ----------------------------------------------------------------------- */
int start1(char *arg)
{
   int kid_pid, status; 

   debugConsole("start1(): at beginning\n");
   check_kernel_mode("start1");

   /* Disable interrupts */
   disableInterrupts();

   /* Initialize the mail box table, slots, & other data structures.
    * Initialize int_vec and sys_vec, allocate mailboxes for interrupt
    * handlers.  Etc... */

   /* Enable interrupts */
   enableInterrupts();

   /* Create a process for start2, then block on a join until start2 quits */
   debugConsole("start1(): fork'ing start2 process\n");
   
   kid_pid = fork1("start2", start2, NULL, 4 * USLOSS_MIN_STACK, 1);
   if ( join(&status) != kid_pid ) {
      console("start2(): join returned something other than start2's pid\n");
   }

   return 0;
} /* start1 */


/* ------------------------------------------------------------------------
   Name - MboxCreate
   Purpose - gets a free mailbox from the table of mailboxes and initializes it 
   Parameters - maximum number of slots in the mailbox and the max size of a msg
                sent to the mailbox.
   Returns - -1 to indicate that no mailbox was created, or a value >= 0 as the
             mailbox id.
   Side Effects - initializes one element of the mail box array. 
   ----------------------------------------------------------------------- */
int MboxCreate(int slots, int slot_size)
{
} /* MboxCreate */


/* ------------------------------------------------------------------------
   Name - MboxSend
   Purpose - Put a message into a slot for the indicated mailbox.
             Block the sending process if no slot available.
   Parameters - mailbox id, pointer to data of msg, # of bytes in msg.
   Returns - zero if successful, -1 if invalid args.
   Side Effects - none.
   ----------------------------------------------------------------------- */
int MboxSend(int mbox_id, void *msg_ptr, int msg_size)
{
} /* MboxSend */


/* ------------------------------------------------------------------------
   Name - MboxReceive
   Purpose - Get a msg from a slot of the indicated mailbox.
             Block the receiving process if no msg available.
   Parameters - mailbox id, pointer to put data of msg, max # of bytes that
                can be received.
   Returns - actual size of msg if successful, -1 if invalid args.
   Side Effects - none.
   ----------------------------------------------------------------------- */
int MboxReceive(int mbox_id, void *msg_ptr, int msg_size)
{
} /* MboxReceive */


int check_io(){
    return 0; 
}

//convenience function to check kernel mode 
void check_kernel_mode(char *message) {
  if ((PSR_CURRENT_MODE & psr_get()) == 0) {
    console("Kernel mode check fail - %s\n", message);
    halt(1);
  }
}

//convenience function for debug output with USLOSS
void debugConsole(char *message){
   if (DEBUG2 && debugflag2)
      console(message);
}

//Disables the interrupts.
void disableInterrupts(void) {
  /* check kernel mode */
  check_kernel_mode("disable interrupt function");

  /* perform bitwise to set disable */
  psr_set(psr_get() & ~PSR_CURRENT_INT);
} /* disableInterrupts */

//Enables the interrupts.
void enableInterrupts(void) {
  /* Turn the interrupts ON if we are in kernel mode */
  check_kernel_mode("enable interrupt function");

  psr_set(psr_get() | PSR_CURRENT_INT);
} /*enableInterrupts*/