#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

pte_t* walkpgdir(pde_t *pgdir, const void *va, int alloc);
int mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm);
char *mem;
uint a;

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

//follow the same pattern as FIFO but we keep removing from head
//that is where the LRU process is stored after updates from the 
//time interrupt
uint
lru(void* va){
   cprintf("\nFAULTED VA 0x%x\n\n", va);
  // cprintf("I am inside FIFO\n");
  pte_t *pte = walkpgdir(myproc()->pgdir, myproc()->psyc[myproc()->head].va, 0);
 // cprintf("HEAD va address 0x%x\n", myproc()->psyc[myproc()->head].va);
  //cprintf("HEAD pa address 0x%x\n", PTE_ADDR(pte));
  // cprintf("HEAD va BACK TRANSLATED rom pa 0x%x\n", P2V(PTE_ADDR(pte)));

  if(pte == 0){
    panic("ERROR walkpgdir");
    exit();
  }
  //writeToSwapFile
  uint offset = ((uint)(myproc()->swap_idx))*PGSIZE;
  // cprintf("head = %d",myproc()->head);
  if(writeToSwapFile(myproc(), (char*)(pte), offset, PGSIZE) < 0){
    // panic("ERROR writeToSwapFile");
    exit();
    return -1;
  }

  //set pte as not present 
  *pte &= ~PTE_P;
  //set pte as paged out
  *pte |= PTE_PG; 
  //decresease pages in physical memory
  myproc()->total_psyc = myproc()->total_psyc - 1;

  //added to swap space
  myproc()->swapspace[myproc()->swap_idx].va =  myproc()->psyc[myproc()->head].va;
  // keep the count of page faults only if it has been swaped out
  // if it has not been swaped out it must be zero
  // myproc()->swapspace[myproc()->swap_idx].count += 1;  
  // cprintf("page swap count: %d\n", myproc()->swapspace[myproc()->swap_idx].count);
  myproc()->swap_idx = (myproc()->swap_idx + 1) % MAX_TOTAL_PAGES;
  //myproc()->head = (myproc()->head + 1) % MAX_PSYC_PAGES; //make sure dont exceed the amount of physical pages
  //link pte of fault page to physical frame of victims address
  //memset((char*)(*pte), 0, PGSIZE);
 if(mappages(myproc()->pgdir, va, PGSIZE, PTE_ADDR(*pte), PTE_W|PTE_U) < 0){
    kfree((char*)(*pte));
  }
  //update references
  lcr3(V2P(myproc()->pgdir));
  // cprintf("EXITING FIFO\n");
  return (uint)PTE_ADDR(*pte);
}

//PAGEBREAK: 41
uint
fifo(void* va){
  cprintf("\nFAULTED VA 0x%x\n\n", va);
  // cprintf("I am inside FIFO\n");
  pte_t *pte = walkpgdir(myproc()->pgdir, myproc()->psyc[myproc()->head].va, 0);
 // cprintf("HEAD va address 0x%x\n", myproc()->psyc[myproc()->head].va);
  //cprintf("HEAD pa address 0x%x\n", PTE_ADDR(pte));
  // cprintf("HEAD va BACK TRANSLATED rom pa 0x%x\n", P2V(PTE_ADDR(pte)));

  if(pte == 0){
    panic("ERROR walkpgdir");
    exit();
  }
  //writeToSwapFile
  uint offset = ((uint)(myproc()->swap_idx))*PGSIZE;
  // cprintf("head = %d",myproc()->head);
  if(writeToSwapFile(myproc(), (char*)(pte), offset, PGSIZE) < 0){
    // panic("ERROR writeToSwapFile");
    exit();
    return -1;
  }

  //set pte as not present 
  *pte &= ~PTE_P;
  //set pte as paged out
  *pte |= PTE_PG; 
  //decresease pages in physical memory
  myproc()->total_psyc = myproc()->total_psyc - 1;

  //added to swap space
  myproc()->swapspace[myproc()->swap_idx].va =  myproc()->psyc[myproc()->head].va;
  // keep the count of page faults only if it has been swaped out
  // if it has not been swaped out it must be zero
  // myproc()->swapspace[myproc()->swap_idx].count += 1;  
  // cprintf("page swap count: %d\n", myproc()->swapspace[myproc()->swap_idx].count);
  myproc()->swap_idx = (myproc()->swap_idx + 1) % MAX_TOTAL_PAGES;
  myproc()->head = (myproc()->head + 1) % MAX_PSYC_PAGES; //make sure dont exceed the amount of physical pages
  //link pte of fault page to physical frame of victims address
 //memset((char*)(*pte), 0, PGSIZE);
 if(mappages(myproc()->pgdir, va, PGSIZE, PTE_ADDR(*pte), PTE_W|PTE_U) < 0){
    kfree((char*)(*pte));
  }
  //update references
  lcr3(V2P(myproc()->pgdir));
  // cprintf("EXITING FIFO\n");
  return (uint)PTE_ADDR(*pte);
}

// random number generator
static unsigned long X = 1;

int random_g() {
  unsigned long a = 1103515245, c = 12345;
  X = a * X + c;
  return ((unsigned int)(X/65536) % 32768) % 15; // mod 15 because of 15 max pages
}

uint
rand(void* va){

  cprintf("\nFAULTED VA 0x%x\n\n", va);

  int rand_num = random_g();  // retrieve a random number
  cprintf("---Random Number =  %d\n",  rand_num); // print random number

  // cprintf("I am inside RAND\n");
  char* temp_va = myproc()->psyc[rand_num].va;

  pte_t *pte = walkpgdir(myproc()->pgdir, temp_va, 0);
  uint temp_pte = PTE_ADDR(*pte);
  cprintf("addr inside rand: 0x%x picked victim: 0x%x\n", temp_va, (char*)temp_pte);
  if(pte == 0){
    panic("ERROR walkpgdir");
    exit();
  }

  //writeToSwapFile
  uint offset = ((uint)(myproc()->swap_idx))*PGSIZE;

  if(writeToSwapFile(myproc(), (char*)(pte), offset, PGSIZE) < 0){
    // panic("ERROR writeToSwapFile");
    exit();
    return -1;
  }
  //set pte as not present
  *pte &= ~PTE_P;
  //set pte as paged out
  *pte |= PTE_PG;
  //decresease pages in physical memory
  myproc()->total_psyc = myproc()->total_psyc - 1;

  //added to swap space
  myproc()->swapspace[myproc()->swap_idx].va = myproc()->psyc[rand_num].va;;
  myproc()->swap_idx = (myproc()->swap_idx + 1) % MAX_TOTAL_PAGES;
  myproc()->tail = rand_num; // to insert in the available spot
  //link pte of fault page to physical frame of victims address
  if(mappages(myproc()->pgdir, va, PGSIZE, temp_pte, PTE_W|PTE_U) < 0){
    kfree((char*)(pte));
  }
      //cprintf("SELECTED: victim: 0x%x\n", (char*)PGROUNDDOWN(rcr2()));
      //readFromSwapFile(myproc(), (char*)PGROUNDDOWN(rcr2()), offset, PGSIZE);
 //update references
  lcr3(V2P(myproc()->pgdir));
  // cprintf("EXITING FIFO\n");
  return temp_pte;
} //end rand()

void
addtostruct(void* a){
  if(myproc()->total_psyc == 0){
    myproc()->head = 0;
    myproc()->tail = 0;
    myproc()->swap_idx = 0;
    myproc()->total_page_out = 0;
    myproc()->swaps_count = 0;
    createSwapFile(myproc());
  }
  myproc()->psyc[myproc()->tail].va = a;


  //cprintf("address:0x%x head: %d insert tail:%d", myproc()->psyc[myproc()->tail].va, myproc()->head,  myproc()->tail);
  myproc()->tail = (myproc()->tail + 1) % MAX_PSYC_PAGES;
 // cprintf(" new tail:%d\n", myproc()->tail);
}

void
trap(struct trapframe *tf)
{

  uint stored_va = PGROUNDDOWN(rcr2());

  if(tf->trapno == T_SYSCALL){

    if(myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
    if(myproc()->killed)
      exit();
    return;
  }

  // PAGE FAULTS:
  // handle Swapping
  if(tf->trapno == T_PGFLT){
 // cprintf("\nSTORED VA 0x%x\n\n", stored_va);
    myproc()->page_faults = myproc()->page_faults + 1;
    // cprintf("page fault\n");
    //if exceeds max total pages kill the process
    if( myproc()->total_pages >= MAX_TOTAL_PAGES){
     // kill(myproc()->pid);
      myproc()->killed = 1;
      return;
    }
   // cprintf("Proc id: %d Total pages: %d Total pages out: %d page faults: %d\n", myproc()->pid, myproc()->total_pages, myproc()->total_page_out,  myproc()->page_faults);
   
    //retrieve faulted va 
    // pde_t *pde;
    // pde = &myproc()->pgdir[PDX(stored_va)];
    pte_t *pte = walkpgdir(myproc()->pgdir, (char*)(stored_va), 0);

    char *mem;
    uint victim = (uint)0;

    //was faulted va swapped out?
    if(!(*pte & PTE_PG)){
      // faulted va was not sapped out
      // increase number of tota pages of the process
      myproc()->total_pages = myproc()->total_pages + 1;
    }

    // cprintf("physical pages: %d\n",  myproc()->psyc_pages);
    // allocate new block from existing free memory
    if(myproc()->total_psyc < MAX_PSYC_PAGES){

      /* EXTRA OWN CODE TESTING */
      //add to data structure
      addtostruct((char*)(stored_va));
      /////////////////////////////////////////

      //increase number of physical pages 
      myproc()->total_psyc = myproc()->total_psyc + 1;
      // call kalloc to get a new physical frame
      mem = kalloc();
      if(mem == 0){
        panic("out of memory\n");
      }
      //call mappages to map the pte to the allocated frame address
      if(mappages(myproc()->pgdir, (char*)(stored_va), PGSIZE, V2P(mem), PTE_W|PTE_U) < 0){
        kfree(mem);
      }
      //make sure all PTE_P bits are zero
      //memset(mem, 0, PGSIZE);

    }
    else{
      // swap out based on replacement algorithm
      // swapout();
      // implement replacement algorithm
      // if fifo
      // writeToSwapFile(myproc(), );
      #if FIFO
        victim = fifo((void*)(stored_va));
        cprintf("FIFO SELECTED: victim: 0x%x\n", (char*)victim);
      #endif

      #if LRU
        victim = lru((void*)(stored_va));
        cprintf("LRU SELECTED\n");
      #endif

      #if RAND
        victim = rand((void*)(stored_va));
        cprintf("RAND SELECTED: victim: 0x%x\n", (char*)victim);
      #endif

      //increase number of page outs 
      myproc()->total_page_out = myproc()->total_page_out + 1;
      myproc()->swaps_count += 1; // number of swaps


    // if page was paged out retrieve it
    if(*pte & PTE_PG){

      cprintf("i am inside pg\n");
      int k = 0;
      uint offset;
      int f = 0;

      for(k = 0; k < MAX_TOTAL_PAGES; k++){
        if(myproc()->swapspace[k].va == (void*)(stored_va)) {
          // cprintf("SWAP PAGE FOUND");
          f = 1;
          break;
        }
      }

      if(f == 0){
        panic("PAGE NOT FOUND");
        exit();
      }

      offset = ((uint)k)*PGSIZE;

      readFromSwapFile(myproc(), (char*)victim, offset, PGSIZE);

      //we retrieved a page that was paged out
      //so decrease pages that were paged out
      myproc()->total_page_out = myproc()->total_page_out - 1;
      //set pte as present 
      *pte |= PTE_P;
      //set pte as not paged out
      *pte &= ~PTE_PG; 

      addtostruct((char*)(stored_va));
      //delete page from swapspace
      myproc()->swapspace[k].va  = (void*)-1;
    }
    else{
      //add not swapped out paged into the avialable block
      addtostruct((char*)(stored_va));

      // myproc()->total_pages = myproc()->total_pages + 1;
      //increase number of physical pages 
      myproc()->total_psyc = myproc()->total_psyc + 1;

      // if(mappages(myproc()->pgdir, (char*)(stored_va), PGSIZE, V2P(victim), PTE_W|PTE_U) < 0){
      //   kfree((char*)victim);
      // }
      //memset((void*)victim, 0, PGSIZE);
      }

    }
    //update references
    lcr3(V2P(myproc()->pgdir));
    return;

  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }

  // NEEDED FOR UPDATING THE DATA STRUCTURE BASED ON 
  // RECENT ACCESS
  #if LRU
    if(myproc() && myproc()->state == RUNNING){
      //record PTEs with Access
      char* array[15];
      int count = 0;
      int j;
      pte_t * pte = (pte_t)0;
      //record all addresses with accesses
      for(j = 0; j < myproc()->total_psyc; j++){
        pte = walkpgdir(myproc()->pgdir, myproc()->psyc[j].va, 0);
        if (*pte & PTE_A){
          array[count] = myproc()->psyc[j].va;
          count+=1;
        }
      }
      
      //update data structure
      if((count > 0)&& (myproc()->total_psyc > 0)){
        //cprintf("I AM UPDATING count: %d\n", count);
        int w;
        for(w = 0; w < count; w++){
          int indx;
          for(indx = 0; indx <= myproc()->tail; indx++){
            if(myproc()->psyc[indx].va == array[count]){
              break;
            }
          }

          int t_idx;
          for(t_idx = indx; t_idx < myproc()->tail; t_idx++){
            myproc()->psyc[t_idx].va = myproc()->psyc[t_idx + 1].va;
          }
          myproc()->psyc[myproc()->tail].va = array[count];
        }
      }

      //reset all accesses
      for(j = 0; j < myproc()->total_psyc; j++){
        pte = walkpgdir(myproc()->pgdir, myproc()->psyc[j].va, 0);
        if (*pte & PTE_A){
          *pte &= ~PTE_A;
        }
      }
    }
  #endif

    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;

  
  //PAGEBREAK: 13
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(myproc() && myproc()->state == RUNNING &&
     tf->trapno == T_IRQ0+IRQ_TIMER)
    yield();

  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
}
