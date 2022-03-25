/* ------------------------------------------------------------------------
   Kyle AuBuchon & Juan Gonzalez
   phase2.c
   Applied Technology
   College of Applied Science and Technology
   The University of Arizona
   CSCV 452

   ------------------------------------------------------------------------ */

#include <phase2.h>

#include <phase1.h>

#include <usloss.h>

#include <string.h>

#include "message.h"


/* ------------------------- Prototypes ----------------------------------- */
int start1(char * );
extern int start2(char * );
void check_kernel_mode(char * proc);
void disableInterrupts();
void enableInterrupts();
int check_io();
void debugOut(char * debugStatement);

void zeroMbox(int mailbox_ID);
void zeroSlot(int slot_id);
void zeroMboxSlot(int pid);

//six  routines  are MboxCreate,  MboxSend,  MboxReceive,  MboxCondSend,  MboxCondReceive, and MboxRelease. 
int MboxCreate(int slots, int slot_size);
int MboxSend(int mbox_id, void * msg_ptr, int msg_size);
int MboxReceive(int mbox_id, void * msg_ptr, int max_msg_size);

int MboxRelease(int mailboxID);
int MboxCondSend(int mailboxID, void * msg, int msg_size);
int MboxCondReceive(int mailboxID, void * msg, int max_msg_size);

slot_ptr createSlot(int index, int mbox_id, void * msg, int msg_size);
int getSlot();
int addSlotToList(slot_ptr slot, mail_box_ptr mailbox_ptr);

int waitdevice(int type, int unit, int * status);

/* -------------------------- Globals ------------------------------------- */
//int debugflag2 = 0;
int debugflag2 = 0;

/* the mail boxes */
mail_box MailBoxTable[MAXMBOX];

/* track of slots */
mail_slot SlotTable[MAXSLOTS];

/* proc table*/
mbox_proc MboxProcessTable[MAXPROC];

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
int start1(char * arg) {
    debugOut("start1(): at beginning\n");

    int kid_pid, status;

    check_kernel_mode("start1");

    /* Disable interrupts */
    disableInterrupts();

    /* Initialize the mail box table, slots, & other data structures.
     * Initialize int_vec and sys_vec, allocate mailboxes for interrupt
     * handlers.  Etc... */

    // init mail box tracking
    for (int i = 0; i < MAXMBOX; i++) {
        MailBoxTable[i].mbox_id = i;
        zeroMbox(i);
    }
    // init slot array
    for (int i = 0; i < MAXSLOTS; i++) {
        SlotTable[i].slot_id = i;
        zeroSlot(i);
    }

    // init process table
    for (int i = 0; i < MAXPROC; i++) {
        zeroMboxSlot(i);
    }

    enableInterrupts();

    /* Create a process for start2, then block on a join until start2 quits */
    debugOut("start1(): fork'ing start2 process\n");
    kid_pid = fork1("start2", start2, NULL, 4 * USLOSS_MIN_STACK, 1);
    if (join( & status) != kid_pid) {
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
int MboxCreate(int slots, int slot_size) {

    check_kernel_mode("MboxCreate");
    disableInterrupts();

    // check if mailboxes are availabe
    if (slots < 0) {
        debugOut("No mailboxes avaliable\n");
        enableInterrupts();
        return -1;
    }

    // check if slot size is correct
    if (slot_size < 0 || slot_size > MAX_MESSAGE) {
        debugOut("Incorrect slot size in MboxCreate\n");
        enableInterrupts();
        return -1;
    }

    // Create Mailbox in next available entry in MailboxTable
    for (int i = 0; i < MAXMBOX; i++) {
        if (MailBoxTable[i].status == EMPTY) {
            MailBoxTable[i].status = USED;
            MailBoxTable[i].total_slots = slots;
            MailBoxTable[i].total_slots_used = 0;
            MailBoxTable[i].slot_size = slot_size;
            enableInterrupts();
            return i;
        }
    }
    enableInterrupts();
    return -1;
} /* MboxCreate */

/* ------------------------------------------------------------------------
   Name - MboxSend
   Purpose - Put a message into a slot for the indicated mailbox.
             Block the sending process if no slot available.
   Parameters - mailbox id, pointer to data of msg, # of bytes in msg.
   Returns - zero if successful, -1 if invalid args.
   Side Effects - none.
   ----------------------------------------------------------------------- */
int MboxSend(int mbox_id, void * msg_ptr, int msg_size) {
    check_kernel_mode("MboxSend");
    disableInterrupts();

    // Check if parameters are correct
    if (mbox_id > MAXMBOX || mbox_id < 0) {
        enableInterrupts();
        return -1;
    }

    // Check if mailbox being sent message exists
    if (MailBoxTable[mbox_id].status == EMPTY) {
        enableInterrupts();
        return -1;
    }

    mail_box_ptr ptr = & MailBoxTable[mbox_id];

    // Check if room in target mailbox for msg
    if (msg_size > ptr -> slot_size) {
        enableInterrupts();
        return -1;
    }

    // add to process table
    int pid = getpid();
    MboxProcessTable[pid % MAXPROC].pid = pid;
    MboxProcessTable[pid % MAXPROC].status = ACTIVE;
    MboxProcessTable[pid % MAXPROC].msg = msg_ptr;
    MboxProcessTable[pid % MAXPROC].msg_size = msg_size;

    // Block on no available slots and add to block send list
    if (ptr -> total_slots <= ptr -> total_slots_used && ptr -> block_rlist == NULL) {

        // Placing process in blocked send list
        if (ptr -> block_slist == NULL) {
            ptr -> block_slist = & MboxProcessTable[pid % MAXPROC];
        } else {
            mbox_proc_ptr temp = ptr -> block_slist;
            while (temp -> next_Block_Send != NULL) {
                temp = temp -> next_Block_Send;
            }
            temp -> next_Block_Send = & MboxProcessTable[pid % MAXPROC];
        }
        block_me(SEND_BLOCK);

        // check if the process was released
        if (MboxProcessTable[pid % MAXPROC].mbox_Released) {
            enableInterrupts();
            return -3;
        }
        return is_zapped() ? -3 : 0;
    }

    // Check if process on receive blocked list
    if (ptr -> block_rlist != NULL) {

        // message is bigger than receive size
        if (msg_size > ptr -> block_rlist -> msg_size) {
            ptr -> block_rlist -> status = FAILED;
            int pid = ptr -> block_rlist -> pid;
            ptr -> block_rlist = ptr -> block_rlist -> next_Block_Receive;
            unblock_proc(pid);
            enableInterrupts();
            return -1;
        }

        // Copy message to receive buffer
        memcpy(ptr -> block_rlist -> msg, msg_ptr, msg_size);
        ptr -> block_rlist -> msg_size = msg_size;

        int receivePID = ptr -> block_rlist -> pid;
        ptr -> block_rlist = ptr -> block_rlist -> next_Block_Receive;
        unblock_proc(receivePID);
        enableInterrupts();
        return is_zapped() ? -3 : 0;
    }

    // find empty slot in SlotTable
    int slot = getSlot();
    if (slot == -2) {
        console("MboxSend(): No slots available. Halting...\n");
        halt(1);
    }

    // Initialize and add slot to global slotlist
    slot_ptr new_slot = createSlot(slot, ptr -> mbox_id, msg_ptr, msg_size);

    // add slot to mailbox slotlist
    addSlotToList(new_slot, ptr);

    enableInterrupts();
    return is_zapped() ? -3 : 0;

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
int MboxReceive(int mbox_id, void * msg_ptr, int max_msg_size) {
    check_kernel_mode("MboxReceive");
    disableInterrupts();

    // check that mailbox to receive exists
    if (MailBoxTable[mbox_id].status == EMPTY) {
        enableInterrupts();
        return -1;
    }

    mail_box_ptr ptr = & MailBoxTable[mbox_id];

    // check that max_msg_size is valid
    if (max_msg_size < 0) {
        enableInterrupts();
        return -1;
    }

    // add to process table
    int pid = getpid();
    MboxProcessTable[pid % MAXPROC].pid = pid;
    MboxProcessTable[pid % MAXPROC].status = ACTIVE;
    MboxProcessTable[pid % MAXPROC].msg = msg_ptr;
    MboxProcessTable[pid % MAXPROC].msg_size = max_msg_size;

    slot_ptr slotPTR = ptr -> slot_list;

    // no full slots for mailbox
    if (slotPTR == NULL) {

        // add receive process to its own blocked receive list
        if (ptr -> block_rlist == NULL) {
            ptr -> block_rlist = & MboxProcessTable[pid % MAXPROC];
        } else {
            mbox_proc_ptr temp = ptr -> block_rlist;

            while (temp -> next_Block_Receive != NULL) {
                temp = temp -> next_Block_Receive;
            }
            temp -> next_Block_Receive = & MboxProcessTable[pid % MAXPROC];
        }

        // block until message recieved
        block_me(RECEIVE_BLOCK);

        // check mailbaox status - released/zapped
        if (MboxProcessTable[pid % MAXPROC].mbox_Released || is_zapped()) {
            enableInterrupts();
            return -3;
        }

        // never recieved message
        if (MboxProcessTable[pid % MAXPROC].status == FAILED) {
            enableInterrupts();
            return -1;
        }
        enableInterrupts();
        return MboxProcessTable[pid % MAXPROC].msg_size;

    } else {

        if (slotPTR -> msg_size > max_msg_size) {
            enableInterrupts();
            return -1;
        }

        // move in received
        memcpy(msg_ptr, slotPTR -> msg, slotPTR -> msg_size);
        ptr -> slot_list = slotPTR -> nslot;
        int msgSize = slotPTR -> msg_size;

        // zero out and reduce total
        zeroSlot(slotPTR -> slot_id);
        ptr -> total_slots_used--;

        // check mailbox message on blocked send list
        if (ptr -> block_slist != NULL) {

            int index = getSlot();

            // init slot 
			// add to mailbox slotlist
            slot_ptr new_slot = createSlot(index, ptr -> mbox_id, ptr -> block_slist -> msg, ptr -> block_slist -> msg_size);
            addSlotToList(new_slot, ptr);
            int pid = ptr -> block_slist -> pid;
            ptr -> block_slist = ptr -> block_slist -> next_Block_Send;
            unblock_proc(pid);
        }
        enableInterrupts();
        return is_zapped() ? -3 : msgSize;
    }
} /* MboxReceive */
   
/* ------------------------------------------------------------------------
Releases a previously created mailbox. Any process waiting on the mailbox should be zap’d. 
Note, however, that zap’ing does not work in every case. It would work for a high priority 
process releasing low priority processes from the mailbox, but not the other way around 2. 
You will need to devise a different means of handling processes that are blocked on a mailbox 
being released. Essentially, you will need to have a blocked process return -3 from the send 
or receive that caused it to block. You will need to have the process that called MboxRelease 
unblock all the blocked process. When each of these processes awake from the block_me call 
inside send or receive, they will need to “notice” that the mailbox has been released...3 
Return values: 
-3:  process was zap’d while releasing the mailbox. 
-1:  the mailboxID is not a mailbox that is in use.
   ----------------------------------------------------------------------- */
int MboxRelease(int mbox_id) {
    check_kernel_mode("MboxRelease");
    disableInterrupts();

    // check mbox_id
    if (mbox_id < 0 || mbox_id >= MAXMBOX) {
        enableInterrupts();
        return -1;
    }

    // verify creation
    if (MailBoxTable[mbox_id].status == EMPTY) {
        enableInterrupts();  
        return -1;
    }

    mail_box_ptr ptr = & MailBoxTable[mbox_id];

    // check processes 
    if (ptr -> block_rlist == NULL && ptr -> block_slist == NULL) {
        zeroMbox(mbox_id);
        enableInterrupts();
        return is_zapped() ? -3 : 0;
    } else {
        ptr -> status = EMPTY;

        // blocked processes released
        while (ptr -> block_slist != NULL) {
            ptr -> block_slist -> mbox_Released = 1;
            int pid = ptr -> block_slist -> pid;
            ptr -> block_slist = ptr -> block_slist -> next_Block_Send;
            unblock_proc(pid);
            disableInterrupts();
        }
        while (ptr -> block_rlist != NULL) {
            ptr -> block_rlist -> mbox_Released = 1;
            int pid = ptr -> block_rlist -> pid;
            ptr -> block_rlist = ptr -> block_rlist -> next_Block_Receive;
            unblock_proc(pid);
            disableInterrupts();
        }
    }
    zeroMbox(mbox_id);
    enableInterrupts();
    return is_zapped() ? -3 : 0;
} /* MboxRelease */

/* ------------------------------------------------------------------------
Conditionally receive a message from a mailbox. Do not block the invoking process in cases 
where there are no messages to receive. Instead, return a -2 if there are no messages in the 
mailbox or, in the case of a zero-slot mailbox, there is no sender blocked on the send function. 
Return values: 
-3:  process is zap’d. 
-2:  mailbox empty, no message to receive. 
-1:  illegal values given as arguments; or, message sent is too large for 
receiver’s buffer (no data copied in this case). 
≥ 0:  the size of the message received.
   ----------------------------------------------------------------------- */
int MboxCondSend(int mbox_id, void * msg_ptr, int msg_size) {
    check_kernel_mode("MboxCondSend");
    disableInterrupts();

    // check if mbox id is valid
    if (mbox_id > MAXMBOX || mbox_id < 0) {
        enableInterrupts();
        return -1;
    }

    mail_box_ptr ptr = & MailBoxTable[mbox_id];

    // check that receive buffer is large enough
    if (msg_size > ptr -> slot_size) {
        enableInterrupts();
        return -1;
    }

    // Add process to ProcTable
    int pid = getpid();
    MboxProcessTable[pid % MAXPROC].pid = pid;
    MboxProcessTable[pid % MAXPROC].status = ACTIVE;
    MboxProcessTable[pid % MAXPROC].msg = msg_ptr;
    MboxProcessTable[pid % MAXPROC].msg_size = msg_size;

    // Check that there is an open slot in mailbox
    if (ptr -> total_slots == ptr -> total_slots_used) {
        return -2;
    }

    // check if process is on the receive blocked list
    if (ptr -> block_rlist != NULL) {
        if (msg_size > ptr -> block_rlist -> msg_size) {
            enableInterrupts();
            return -1;
        }

        // copy message into message buffer
        memcpy(ptr -> block_rlist -> msg, msg_ptr, msg_size);
        ptr -> block_rlist -> msg_size = msg_size;
        int receivePID = ptr -> block_rlist -> pid;
        ptr -> block_rlist = ptr -> block_rlist -> next_Block_Receive;
        unblock_proc(receivePID);
        enableInterrupts();
        return is_zapped() ? -3 : 0;
    }

    // Find an empty slot in Slot table
    int slot = getSlot();
    if (slot == -2) {
        return -2;
    }

    // Initialize slot and add it to mailbox slot list
    slot_ptr new_slot = createSlot(slot, ptr -> mbox_id, msg_ptr, msg_size);
    addSlotToList(new_slot, ptr);

    enableInterrupts();
    return is_zapped() ? -3 : 0;
} /* MboxCondSend*/

/* ------------------------------------------------------------------------
MboxCondReceive
Conditionally receive a message from a mailbox. Do not block the invoking process in cases 
where there are no messages to receive. Instead, return a -2 if there are no messages in the 
mailbox or, in the case of a zero-slot mailbox, there is no sender blocked on the send function. 
Return values: 
-3:  process is zap’d. 
-2:  mailbox empty, no message to receive. 
-1:  illegal values given as arguments; or, message sent is too large for 
receiver’s buffer (no data copied in this case). 
≥ 0:  the size of the message received.
   ----------------------------------------------------------------------- */
int MboxCondReceive(int mbox_id, void * msg_ptr, int msg_size) {
    check_kernel_mode("MboxCondReceive");
    disableInterrupts();

    // check that mailbox to receive exists
    if (MailBoxTable[mbox_id].status == EMPTY) {
        enableInterrupts();
        return -1;
    }

    mail_box_ptr ptr = & MailBoxTable[mbox_id];

    // check that msg size is valid
    if (msg_size < 0) {
        enableInterrupts();
        return -1;
    }

    // Add process to Proc Table
    int pid = getpid();
    MboxProcessTable[pid % MAXPROC].pid = pid;
    MboxProcessTable[pid % MAXPROC].status = ACTIVE;
    MboxProcessTable[pid % MAXPROC].msg = msg_ptr;
    MboxProcessTable[pid % MAXPROC].msg_size = msg_size;

    slot_ptr slotPTR = ptr -> slot_list;

    // No message in mailbox slots
    if (slotPTR == NULL) {
        enableInterrupts();
        return -2;

    } else {

        // check that message buffer is big enough
        if (slotPTR -> msg_size > msg_size) {
            enableInterrupts();
            return -1;
        }

        // copy message into receive buffer
        memcpy(msg_ptr, slotPTR -> msg, slotPTR -> msg_size);
        ptr -> slot_list = slotPTR -> nslot;
        int msg_size = slotPTR -> msg_size;

        // zero slot and reduce number of used slots
        zeroSlot(slotPTR -> slot_id);
        ptr -> total_slots_used--;

        // check if blocked message on send list
        if (ptr -> block_slist != NULL) {
            int slot = getSlot();

            // initialize slot and add it to mailbox slot
            slot_ptr new_slot = createSlot(slot, ptr -> mbox_id, ptr -> block_slist -> msg, ptr -> block_slist -> msg_size);
            addSlotToList(new_slot, ptr);

            // update process and unblock
            int pid = ptr -> block_slist -> pid;
            ptr -> block_slist = ptr -> block_slist -> next_Block_Send;
            unblock_proc(pid);
        }
        enableInterrupts();
        return is_zapped() ? -3 : msg_size;
    }
} /* MboxCondReceive */

/*
waitdevice
Do a receive operation on the mailbox associated with the given unit of the device type. The 
device types are defined in usloss.h. The appropriate device mailbox is sent a message every 
time an interrupt is generated by the I/O device, except for the clock device which should only 
be sent a message every 100 milliseconds (every 5 interrupts). This routine will be used to 
synchronize with a device driver process in phase 4. waitdevice returns the device’s status 
register in *status. 
Return values: 
-1:  the process was zapped while waiting 
0:  otherwise 
*/
int waitdevice(int type, int unit, int * status) {
    int result = 0;

    /* Check if an existing type is being used */
    if ((type != DISK_DEV) && (type != CLOCK_DEV) && (type != TERM_DEV)) {
        debugOut("waitdevice(): incorrect type. Halting..\n");
        halt(1);
    }

    // different device types grabbed/defined in usloss.h
    switch (type) {
    case CLOCK_DEV:
        result = MboxReceive(unit, status, sizeof(int));
        break;
    case ALARM_DEV:
        result = MboxReceive(unit, status, sizeof(int));
        break;
    case DISK_DEV:
        result = MboxReceive(unit, status, sizeof(int));
        break;
    case TERM_DEV:
        result = MboxReceive(unit, status, sizeof(int));
        break;
    default:
        console("Device type not found\n");
        break;
    }

    if (result == -3) {
        /* we were zapped */
        return (-1);
    } else {
        return 0;
    }

} /*waitdevice*/

void check_kernel_mode(char * proc) {
    if ((PSR_CURRENT_MODE & psr_get()) == 0) {
        console("check_kernel_mode(): called while in user mode by process %s. Halting...\n", proc);
        halt(1);
    }
}

void enableInterrupts() {
    //debugOut("enableInterrupts called\n");
    psr_set(psr_get() | PSR_CURRENT_INT);
}

void disableInterrupts() {
    //debugOut("disableInterrupts called\n");
    psr_set(psr_get() & ~PSR_CURRENT_INT);
}

// zeroes all elements using mbox_id
void zeroMbox(int mbox_id) {
    MailBoxTable[mbox_id].status = EMPTY;
    MailBoxTable[mbox_id].block_rlist = NULL;
    MailBoxTable[mbox_id].block_slist = NULL;
    MailBoxTable[mbox_id].slot_list = NULL;
    MailBoxTable[mbox_id].total_slots = -1;
    MailBoxTable[mbox_id].total_slots_used = -1;
    MailBoxTable[mbox_id].slot_size = -1;

}

// zeroes all elements of the slot id
void zeroSlot(int slot_id) {
    SlotTable[slot_id].status = EMPTY;
    SlotTable[slot_id].nslot = NULL;
    SlotTable[slot_id].mbox_id = -1;

}

// zeroes all slot ids
void zeroMboxSlot(int pid) {
    MboxProcessTable[pid % MAXPROC].status = EMPTY;
    MboxProcessTable[pid % MAXPROC].msg = NULL;
    MboxProcessTable[pid % MAXPROC].next_Block_Receive = NULL;
    MboxProcessTable[pid % MAXPROC].next_Block_Send = NULL;
    MboxProcessTable[pid % MAXPROC].pid = -1;
    MboxProcessTable[pid % MAXPROC].msg_size = -1;
    MboxProcessTable[pid % MAXPROC].mbox_Released = 0;
}

// find index of next slot, if none are available returns -2
int getSlot() {
    for (int i = 0; i < MAXSLOTS; i++) {
        if (SlotTable[i].status == EMPTY) {
            return i;
        }
    }
    return -2;
}

// init new slot
slot_ptr createSlot(int index, int mbox_id, void * msg, int msg_size) {
    SlotTable[index].mbox_id = mbox_id;
    SlotTable[index].status = USED;
    memcpy(SlotTable[index].msg, msg, msg_size);
    SlotTable[index].msg_size = msg_size;
    return &SlotTable[index];
}

int check_io() {
    return 0;
}

// add slot
int addSlotToList(slot_ptr new_slot, mail_box_ptr ptr) {
    slot_ptr head = ptr -> slot_list;
    if (head == NULL) {
        ptr -> slot_list = new_slot;
    } else {
        while (head -> nslot != NULL) {
            head = head -> nslot;
        }
        head -> nslot = new_slot;
    }
    return ptr -> total_slots_used++;
}

// convenience debug func
void debugOut(char * debugStatement) {
    if (DEBUG2 && debugflag2) {
        console(debugStatement);
    }
}