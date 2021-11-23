        .title "SCC1 Interrupt Handler - Ethernet"

        .extern quicc_interrupt_handler
        .extern vTaskSwitchContext

        .section .text
        .align 2

        .type SCC1InterruptHandler, @function
        .globl SCC1InterruptHandler
SCC1InterruptHandler:
        /* Save current task context */
        movem.l %d0-%fp, %sp@-
        movea.l (pxCurrentTCB), %a0
        move.l  %sp, %a0@

        /* Set SP to ISR stack */
        /* Before uncommenting this line across multiple ISRs, some kind of
         * synchronisation is required to ensure that the ISR stack is only
         * switched in once during nested interrupts */
        /* movea.l (xISRStackTop), %sp */
        
        /* Call the QUICC interrupt handler to process the interrupt - should
         * return a boolean indicating whether a task switch is required */
        jsr     quicc_interrupt_handler

        /* If a task switch is required, do it */
        beq     0f
        jsr     vTaskSwitchContext

        /* Restore context of next task */
0:      movea.l (pxCurrentTCB), %a0
        movea.l %a0@, %sp
        movem.l %sp@+, %d0-%fp

        rte

        .end
