/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_IP_Private.h"
#include "FreeRTOS_ARP.h"
#include "NetworkBufferManagement.h"
#include "NetworkInterface.h"

/* Port specific includes */
#include "C1600R.h"

/* Ensure that the RX and TX ring sizes are sane. Each BD consumes 8 bytes of
 * DPRAM, of which the first and largest contiguous block is 1536 bytes long
 * (depending on mask revision, 1536 is the minimum.)
 *
 * This provides room for 192 BDs, but for sanity and to leave some room for
 * other peripherals, error out if the total number of BDs exceeds 128. The
 * QUICC doesnt have any specific requirements for the number of BDs in each
 * ring, so the limit is only on the total number of them. */
#if (ipconfigQUICC_RX_RING_SZ + ipconfigQUICC_TX_RING_SZ) > 128
#error "Maximum number of ring entries (ipconfigQUICC_RX_RING_SZ + ipconfigQUICC_TX_RING_SZ) should not exceed 128"
#endif

/* Round up the size to be allocated to each frame buffer to be a multiple of
 * 4 bytes. */
#define BUFFER_SIZE (ipTOTAL_ETHERNET_FRAME_SIZE + ipBUFFER_PADDING)
#define BUFFER_SIZE_ROUNDED_UP ((BUFFER_SIZE + 3) & ~0x3UL)

/* The QUICK manager task should run at max priority to ensure speedy handling
 * of ethernet events */
#define QUICC_TASK_PRIORITY (configMAX_PRIORITIES - 1)

/* Pointers that will be used to address the start of each BD ring */
static volatile __SCCEthRxBufferDescriptor_t *rx_ring;
static volatile __SCCEthTxBufferDescriptor_t *tx_ring;

/* A pointer to space allocated to buffers */
static uint8_t *pkt_buffers = 0;
static uint8_t *pkt_buffers_aligned = 0;

/* Head and tail indexes for RX and TX rings.
 *
 * The rx_tail index points to the oldest frame to be received that is yet to be
 * passed to FreeRTOS.
 *
 * The tx_head index points to the next available BD where a frame to be
 * transmitted may be placed. */
static volatile uint8_t rx_tail = 0;
static volatile uint8_t tx_head = 0;

/* A handle for the task which will manage ongoing operation of the QUICC */
static TaskHandle_t quicc_task_handle = NULL;

/* A timer that is started when ever a buffer fails to be allocated for the
 * RX ring, and queues a ring maintenance event to re-attempt the allocation */
TimerHandle_t quicc_buffer_realloc_timer_handle = NULL;

/* This enum lists all the different types of events that can be handled by the
 * QUICC management task */
typedef enum {
    QUICC_EVT_NONE = 0,
    QUICC_EVT_RX_INCOMPLETE = 0x1,
    QUICC_EVT_RX_COMPLETE = 0x2,
    QUICC_EVT_BUFF_MAINT = 0x4
} quicc_task_event_t;

/* Forward declarations for private API */
static void release_allocated_buffers(void);
static void quicc_task(void * pvParameters);
static void quicc_task_rx_events(quicc_task_event_t event);
static void quicc_task_buff_maint(void);
static void quicc_buffer_realloc_timer(TimerHandle_t xTimer);

/********************************* PUBLIC API *********************************/
void
vNetworkInterfaceAllocateRAMToBuffers(
    NetworkBufferDescriptor_t pxNetworkBuffers[ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS])
{
    /* First attempt to malloc some space to be used as frame buffers. 4 extra
     * bytes are added to ensure that the first buffer address can be aligned
     * to 4 bytes - this is to accommodate FreeRTOS+TCP which likes to have a
     * pointer stored in the padding at the beginning of the frame buffer, and
     * that pointer must be aligned to 4 bytes. As the buffer size to allocate
     * to each frame is already rounded up to 4 bytes, all subsequent buffers
     * will have their pointers aligned to 4 bytes as well. */
    uint8_t *buf;
    uint32_t index;

    pkt_buffers = (uint8_t *)malloc((BUFFER_SIZE_ROUNDED_UP * ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS) + 4);

    configASSERT(pkt_buffers != NULL);

    if (pkt_buffers == NULL) {
        /* malloc did not succeed */
        return;
    }

    /* Align buffer pointer to 4 bytes, but dont overwrite the original pointer
     * as it may be needed later to free() the allocated space */
    pkt_buffers_aligned = (uint8_t *)(((uintptr_t)pkt_buffers + 3) & ~0x3);

    configASSERT(((uintptr_t)pkt_buffers_aligned & 0x3) == 0);

    /* Assign frame buffers to FreeRTOS buffer descriptors */
    for (index = 0; index < ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS; index++) {
        /* BUFFER_SIZE_ROUNDED_UP is a multiple of 4, so as long as
         * pkt_buffers_aligned is aligned to 4 bytes, then all further buffer
         * allocs should be aligned to 4 bytes as well */
        buf = &pkt_buffers_aligned[BUFFER_SIZE_ROUNDED_UP * index];

        /* Beginning of the frame buffer must be aligned to 4 bytes */
        configASSERT(((uintptr_t)buf & 0x3) == 0);

        /* The actual data buffer starts ipBUFFER_PADDING bytes in */
        pxNetworkBuffers[index].pucEthernetBuffer = &buf[ipBUFFER_PADDING];

        /* Store a pointer to the FreeRTOS buffer descriptor at the beginning
         * of the buffer */
        *(uint32_t *)buf = (uint32_t)(&(pxNetworkBuffers[index]));
    }
}

BaseType_t
xNetworkInterfaceInitialise(void)
{
    uint16_t idx;
    NetworkBufferDescriptor_t *buf;
    const uint16_t *mac_addr;
    uint8_t numbufs = 0;

    /* Attempt to create a task that will manage events from the QUICC */
    if (quicc_task_handle == NULL) {
        /* Try to create a timer which is used to re-attempt allocating new
         * buffers to RX ring entries */
        if (quicc_buffer_realloc_timer_handle == NULL) {
            quicc_buffer_realloc_timer_handle = xTimerCreate(
                "RX ring buffer realloc",
                pdMS_TO_TICKS(20),
                pdFALSE,
                NULL,
                quicc_buffer_realloc_timer);

            configASSERT(quicc_buffer_realloc_timer_handle != NULL);

            if (quicc_buffer_realloc_timer_handle == NULL) {
                return pdFALSE;
            }
        }

        /* Now attempt to create the QUICC manager task */
        if (xTaskCreate(quicc_task,
                        "QUICC manager task",
                        4096,
                        NULL,
                        QUICC_TASK_PRIORITY,
                        &quicc_task_handle) != pdPASS) {
            /* Failed to create task */
            return pdFALSE;
        }
    }

    /* Initialise ring pointers. The RX ring is placed at the very beginning of
     * DPRAM, and the TX ring will follow immediately after. */
    rx_ring = (__SCCEthRxBufferDescriptor_t *)DPRBASE;
    tx_ring = (__SCCEthTxBufferDescriptor_t *)&rx_ring[ipconfigQUICC_RX_RING_SZ];

    /* Initialise rings - RX ring first */
    for (idx = 0; idx < ipconfigQUICC_RX_RING_SZ; idx++) {
        /* Get a buffer to assign to descriptor */
        buf = pxGetNetworkBufferWithDescriptor(BUFFER_SIZE_ROUNDED_UP, 0);

        configASSERT(buf != NULL);

        if (buf == NULL) {
            break;
        }

        /* Count a successful buffer acquisition */
        numbufs++;

        /* Configure BD */
        rx_ring[idx].flags.u16 = 0x9000;      /* E and I set */
        rx_ring[idx].LEN = 0;
        rx_ring[idx].DST = (uint8_t *)buf->pucEthernetBuffer;
    }

    if (numbufs != ipconfigQUICC_RX_RING_SZ) {
        /* A buffer acquisition failed, release any allocated bufs and fail */
        release_allocated_buffers();

        return pdFALSE;
    }

    /* The last BD in the ring should have the W bit set */
    idx = ipconfigQUICC_RX_RING_SZ - 1;
    rx_ring[idx].flags.W = 1;

    /* Clear the rx tail index */
    rx_tail = 0;

    /* TX ring next */
    for (idx = 0; idx < ipconfigQUICC_TX_RING_SZ; idx++) {
        /* TX ring doesnt need any buffers allocated until a frame is ready to
         * TX, so just "zeroise" everything */
        tx_ring[idx].flags.u16 = 0;
        tx_ring[idx].LEN = 0;
        tx_ring[idx].SRC = 0;
    }

    /* The last BD in the ring should have the W bit set */
    idx = ipconfigQUICC_TX_RING_SZ - 1;
    tx_ring[idx].flags.W = 1;

    /* Clear tx head index */
    tx_head = 0;

    /* Initialise SCC1 as an ethernet controller */

    /* Configure port pins */
    PAPAR |= 0x0C03;                /* PA0=RXD1, PA1=TXD1, PA10=Eth TCLK,
                                       PA11=Eth RCLK */
    PADIR &= ~0x0C03;
    PAODR &= ~0x0002;

    PBDAT &= ~0xEC00;               /* Clear PB15..10 outputs, excl PB12 which
                                       becomes the TENA pin later */
    PBDAT |= 0x0000;                /* Set any outputs high? */
    PBPAR &= ~0xEC00;               /* PB15..10 should be GPIOs, excl PB12 */
    PBDIR |= 0xE800;                /* PB15..11 should be outputs, excl PB12 */

    PCPAR &= ~0x0030;               /* PC4=CLSN, PC5=RENA */
    PCDIR &= ~0x0030;
    PCSO |= 0x0030;

    /* Configure SCC1 clock sources - no BRG is used, clocks are supplied by
     * the serialiser */
    SICRbits.T1CS = 6;              /* TCLK is CLK3 */
    SICRbits.R1CS = 7;              /* RCLK is CLK4 */
    SICRbits.SC1 = 0;

    /* Set buffer descriptor pointers */
    SCC1RBASE = (uint16_t)((uint32_t)rx_ring);
    SCC1TBASE = (uint16_t)((uint32_t)tx_ring);
    
    /* Setup function codes for SCC SDMA channels */
    SCC1RFCRbits.MOT = 1;       /* Motorola byte ordering */
    SCC1RFCRbits.FC = 8;        /* Function code */
    SCC1TFCRbits.MOT = 1;
    SCC1TFCRbits.FC = 8;

    SCC1MRBLR = 1520;           /* Maximum receive buffer length */

    /* Clear/initialise defaults as (mostly) recommended by the User Manual */
    SCC1C_PRES = 0xFFFFFFFF;
    SCC1C_MASK = 0xDEBB20E3;
    SCC1CRCEC = 0;
    SCC1ALEC = 0;
    SCC1DISFC = 0;
    SCC1PADS = 0;
    SCC1RET_LIM = 15;
    SCC1MFLR = 1518;
    SCC1MINFLR = 64;
    SCC1MAXD1 = 1518;
    SCC1MAXD2 = 1518;
    SCC1GADDR1 = 0;
    SCC1GADDR2 = 0;
    SCC1GADDR3 = 0;
    SCC1GADDR4 = 0;
    SCC1P_PER = 0;
    SCC1IADDR1 = 0;
    SCC1IADDR2 = 0;
    SCC1IADDR3 = 0;
    SCC1IADDR4 = 0;
    SCC1TADDR_L = 0;
    SCC1TADDR_M = 0;
    SCC1TADDR_H = 0;

    /* Configure MAC address - each word is stored in little endian */
    mac_addr = (const uint16_t *)FreeRTOS_GetMACAddress();

    configASSERT(mac_addr != NULL);

    SCC1PADDR1_H = __builtin_bswap16(mac_addr[0]);
    SCC1PADDR1_M = __builtin_bswap16(mac_addr[1]);
    SCC1PADDR1_L = __builtin_bswap16(mac_addr[2]);

    /* Configure SCC1 General Mode Register */
    GSMR1H = 0;
    GSMR1L = 0x1088000C;

    DSR1 = 0xD555;                  /* Frame sync patterns */

    PSMR1 = 0x080B;

    /* Init SCC1 tx/rx params */
    CR = (_CR_OPCODE_SCC_INIT_TX_RX_PARAMS << _CR_OPCODE_POSITION) |
         (_CR_CHNUM_SCC1 << _CR_CHNUM_POSITION) |
         1;

    /* Configure interrupts for SCC1 */
    SCCE1 = 0xFFFF;                 /* Clear all pending events */
    SCCM1 = 0x0009;                 /* RX interrupts only */
    CIMRbits.SCC1 = 1;              /* Enable SCC1 interrupts */

    /* Configure TENA pin, and enable the receiver and transmitter */
    PBPAR |= 0x1000;
    PBDIR |= 0x1000;
    GSMR1L = 0x1088003C;

    return pdTRUE;
}

BaseType_t
xNetworkInterfaceOutput(NetworkBufferDescriptor_t * const pxDescriptor,
                        BaseType_t xReleaseAfterSend)
{
    __SCCEthTxBufferDescriptor_t *td;
    uint8_t *pucEthernetBuffer = NULL;
    NetworkBufferDescriptor_t *pxNetworkBuffer = NULL;

    td = (__SCCEthTxBufferDescriptor_t *)&tx_ring[tx_head];

    if (td->flags.R) {
        /* If the head index is owned by the QUICC then all buffers are in use.
         * Drop the frame.
         *
         * TODO: This could be improved by queueing frames that couldnt be
         *       immediately transmitted */
        vReleaseNetworkBufferAndDescriptor(pxDescriptor);

        return pdFALSE;
    }

    if (td->SRC != NULL) {
        /* The head has an as yet unallocated buffer in it, release it and then
         * the head BD can be used for this frame */

        if (td->flags.u16 & 0x00C2) {
            /* UN || RL || LC - conditions that require the transmitter to be
            * restarted */
            CR = (_CR_OPCODE_SCC_RESTART_TX << _CR_OPCODE_POSITION) |
                    (_CR_CHNUM_SCC1 << _CR_CHNUM_POSITION) |
                    1;
        }

        pucEthernetBuffer = (uint8_t *)td->SRC;
        pxNetworkBuffer = pxPacketBuffer_to_NetworkBuffer((void *)pucEthernetBuffer);

        configASSERT(pxNetworkBuffer != NULL);

        vReleaseNetworkBufferAndDescriptor(pxNetworkBuffer);
        td->flags.u16 &= 0x2000;
    }

    /* Set up td for TX */
    td->LEN = pxDescriptor->xDataLength;
    td->SRC = (void *)pxDescriptor->pucEthernetBuffer;

    /* Set flags, incl R which causes the frame to be transmitted */
    td->flags.u16 |= 0xDC00;        /* Set R, PAD, I, L, TC bits */

    /* Increment and wrap the head index */
    tx_head++;

    if (tx_head == ipconfigQUICC_TX_RING_SZ) {
        tx_head = 0;
    }

    /* In a zero copy driver, xReleaseAfterSend can be ignored, so there is
     * nothing to do in relation to this setting */

    return pdTRUE;
}

BaseType_t
quicc_interrupt_handler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t task_woken = pdFALSE;

    /* Get pended interrupts */
    __SCCEEthbits_t events = { .u16 = SCCE1 };

    if (events.RXB) {
        /* An incomplete frame has been received into a buffer */
        xTaskNotifyFromISR(quicc_task_handle,
                           QUICC_EVT_RX_INCOMPLETE,
                           eSetBits,
                           &task_woken);
        xHigherPriorityTaskWoken |= task_woken;
    }

    if (events.RXF) {
        /* A complete frame has been received into a buffer */
        xTaskNotifyFromISR(quicc_task_handle,
                           QUICC_EVT_RX_COMPLETE,
                           eSetBits,
                           &task_woken);
        xHigherPriorityTaskWoken |= task_woken;
    }

//     if (events.TXE) {
//         /* An error occurred during transmission */
//     }

//     if (events.TXB) {
//         /* A frame has completed transmission */
//     }

//    if (events.BSY) {
//        /* A frame was received but there was no BD available for it, so it was
//         * dropped */
//    }

//    if (events.GRA) {
//        /* The SCC has completed the requested graceful shutdown */
//    }

    /* Clear event/interrupt flags */
    SCCE1 = events.u16;
    CISR = 1 << _CISR_SCC1_POSITION;

    return xHigherPriorityTaskWoken;
}


/********************************* PRIVATE API ********************************/
static void
release_allocated_buffers(void)
{
    /* Return allocated buffers in the RX ring back to FreeRTOS. This is done
     * if xNetworkInterfaceInitialise() fails. */
    uint8_t idx;
    NetworkBufferDescriptor_t *pxNetworkBuffer;
    __SCCEthRxBufferDescriptor_t *rd = NULL;

    for (idx = 0; idx < ipconfigQUICC_RX_RING_SZ; idx++) {
        rd = (__SCCEthRxBufferDescriptor_t *)&rx_ring[idx];

        if (rd->DST != NULL) {
            pxNetworkBuffer = pxPacketBuffer_to_NetworkBuffer(rd->DST);

            configASSERT(pxNetworkBuffer != NULL);

            if (pxNetworkBuffer != NULL) {
                vReleaseNetworkBufferAndDescriptor(pxNetworkBuffer);
            }
        }
    }
}

static void __attribute__ ((noreturn))
quicc_task(void * pvParameters)
{
    quicc_task_event_t events = QUICC_EVT_NONE;

    while (true) {
        /* Wait for some events to be notified */
        xTaskNotifyWait(0, 0xFFFFFFFF, &events, 10);

        if (events == QUICC_EVT_NONE) {
            /* No event bits set */
            continue;
        }

        // if (events & QUICC_EVT_TX_COMPLETE || events & QUICC_EVT_TX_ERROR) {
        //     quicc_task_tx_events();
        // }

        if (events & QUICC_EVT_RX_COMPLETE || events & QUICC_EVT_RX_INCOMPLETE) {
            quicc_task_rx_events(events);
        }

        if (events & QUICC_EVT_BUFF_MAINT) {
            quicc_task_buff_maint();
        }
    }
}

static void
quicc_task_rx_events(quicc_task_event_t events)
{
    /* Handle receiver events
     *
     * Loop through the RX ring and process any frames that have been received.
     */
    uint8_t try_more = true;
    __SCCEthRxBufferDescriptor_t *rd = NULL;
    uint8_t *pucEthernetBuffer = NULL;
    NetworkBufferDescriptor_t *pxNetworkBuffer = NULL;
    IPStackEvent_t xRxEvent = {0};

    while (try_more) {
        rd = (__SCCEthRxBufferDescriptor_t *)&rx_ring[rx_tail];

        if (rd->flags.E) {
            /* BD is empty, no more left to process */
            break;
        }

        pucEthernetBuffer = (uint8_t *)rd->DST;
        pxNetworkBuffer = pxPacketBuffer_to_NetworkBuffer((void *)pucEthernetBuffer);

        configASSERT(pxNetworkBuffer != NULL);

        if (rd->flags.u16 & 0x003F || events & QUICC_EVT_RX_INCOMPLETE) {
            /* Frame contents probably are no good, so reset flags to make the
             * BD available to receive another frame */
            rd->flags.u16 &= 0x2000;
            rd->flags.u16 |= 0x9000;
        } else {
            /* Frame is probably OK, so try to queue it with FreeRTOS */
            xRxEvent.eEventType = eNetworkRxEvent;
            xRxEvent.pvData = (void *)pxNetworkBuffer;

            pxNetworkBuffer->xDataLength = rd->LEN;

            if (rd->flags.L) {
                /* The last BD belonging to a large frame includes 4 bytes for
                 * the CRC, remove these before passing to FreeRTOS */
                pxNetworkBuffer->xDataLength -= 4;
            }

            if (xSendEventStructToIPTask(&xRxEvent, 1) == pdPASS) {
                /* Successfully queued, reset flags and destination pointer
                 * ahead of assigning a new buffer */
                rd->flags.u16 &= 0x2000;
                rd->DST = NULL;
            } else {
                /* Wasnt able to queue with FreeRTOS, so reset the flags to make
                 * the BD available to receive another frame */
                rd->flags.u16 &= 0x2000;
                rd->flags.u16 |= 0x9000;
            }
        }

        /* If the destination pointer has been nulled, try to allocate a new
         * buffer */
        if (rd->DST == NULL) {
            pxNetworkBuffer = pxGetNetworkBufferWithDescriptor(
                BUFFER_SIZE_ROUNDED_UP,
                1);

            configASSERT(pxNetworkBuffer != NULL);

            if (pxNetworkBuffer != NULL) {
                /* Got a buffer */
                rd->DST = pxNetworkBuffer->pucEthernetBuffer;
                rd->flags.u16 |= 0x9000;
            } else {
                /* Didnt get a buffer, start the timer and exit as it might
                 * be unlikely that we will get any more */
                xTimerStart(quicc_buffer_realloc_timer_handle, 0);

                try_more = false;
            }
        }

        /* Increment and wrap the tail pointer */
        rx_tail++;

        if (rx_tail == ipconfigQUICC_RX_RING_SZ) {
            rx_tail = 0;
        }
    }
}

static void
quicc_task_buff_maint(void)
{
    /* If a buffer cannot be allocated in the RX event handler, a timer is
     * started to schedule this buffer maintenance procedure which re-attempts
     * allocating a new buffer */
    uint8_t ctr;
    __SCCEthRxBufferDescriptor_t *rd = NULL;
    NetworkBufferDescriptor_t *pxNetworkBuffer = NULL;

    for (ctr = 0; ctr < ipconfigQUICC_RX_RING_SZ; ctr++) {
        rd = (__SCCEthRxBufferDescriptor_t *)&rx_ring[ctr];

        if (rd->DST == NULL) {
            /* RX BD has a null destination pointer which means the BD is
             * missing a buffer - try to allocate one */
            pxNetworkBuffer = pxGetNetworkBufferWithDescriptor(
                BUFFER_SIZE_ROUNDED_UP,
                1);

            configASSERT(pxNetworkBuffer != NULL);

            if (pxNetworkBuffer != NULL) {
                /* Got a buffer */
                rd->DST = pxNetworkBuffer->pucEthernetBuffer;
                rd->flags.u16 |= 0x9000;
            } else {
                /* Didnt get a buffer, re-start the timer and exit as it might
                 * be unlikely that we will get any more */
                xTimerStart(quicc_buffer_realloc_timer_handle, 0);

                break;
            }
        }
    }
}

static void
quicc_buffer_realloc_timer(TimerHandle_t xTimer)
{
    /* Queue a buffer maintenance event with the QUICC manager task, this will
     * start a process to scan the RX ring and fill in any BDs that have no
     * destination buffer */
    xTaskNotify(quicc_task_handle, QUICC_EVT_BUFF_MAINT, eSetBits);
}
