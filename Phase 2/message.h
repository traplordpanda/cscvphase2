#define DEBUG2 0

#define EMPTY  0
#define USED   1
#define ACTIVE 2
#define FAILED 3

#define SEND_BLOCK 101
#define RECEIVE_BLOCK 102

typedef struct mail_slot *slot_ptr;
typedef struct mailbox mail_box;
typedef struct mbox_proc *mbox_proc_ptr;

typedef struct mailbox *mail_box_ptr;
typedef struct mbox_proc mbox_proc;
typedef struct mail_slot mail_slot;


struct mbox_proc {
	short		 pid;
	int          status;
	void        *msg;
	int          msg_size;
	int			 mbox_Released;
	mbox_proc_ptr next_Block_Send;
	mbox_proc_ptr next_Block_Receive;
};


struct mailbox {
	int			 mbox_id;
	int			 status;
	int			 total_slots;
	int			 total_slots_used;
	int          slot_size;
	mbox_proc_ptr block_slist;
	mbox_proc_ptr block_rlist;
	slot_ptr     slot_list;
      /* other items as needed... */
};

struct mail_slot {
	int          slot_id;
	int		     mbox_id;
	int          status;
	char         msg[MAX_MESSAGE];
	int          msg_size;
	slot_ptr     nslot;
   /* other items as needed... */

};

struct psr_bits {
    unsigned int cur_mode:1;
    unsigned int cur_int_enable:1;
    unsigned int prev_mode:1;
    unsigned int prev_int_enable:1;
    unsigned int unused:28;
};

union psr_values {
   struct psr_bits bits;
   unsigned int integer_part;
};