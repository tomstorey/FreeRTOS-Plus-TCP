/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_IP_Private.h"
#include "FreeRTOS_ARP.h"
#include "NetworkBufferManagement.h"
#include "NetworkInterface.h"

/* Port specific includes */
#include "C2500.h"
#include "LANCE.h"

typedef struct {
    union {
        struct {
            uint16_t PROM:1;
            uint16_t :6;
            uint16_t EMBA:1;
            uint16_t INTL:1;
            uint16_t DRTY:1;
            uint16_t COLL:1;
            uint16_t DTCR:1;
            uint16_t LOOP:1;
            uint16_t DTX:1;
            uint16_t DRX:1;
        };
        struct {
            uint16_t u16;
        };
    } MODE;
    union {
        struct {
            uint16_t PADDR47:1;
            uint16_t PADDR46:1;
            uint16_t PADDR45:1;
            uint16_t PADDR44:1;
            uint16_t PADDR43:1;
            uint16_t PADDR42:1;
            uint16_t PADDR41:1;
            uint16_t PADDR40:1;
            uint16_t PADDR39:1;
            uint16_t PADDR38:1;
            uint16_t PADDR37:1;
            uint16_t PADDR36:1;
            uint16_t PADDR35:1;
            uint16_t PADDR34:1;
            uint16_t PADDR33:1;
            uint16_t PADDR32:1;
            uint16_t PADDR31:1;
            uint16_t PADDR30:1;
            uint16_t PADDR29:1;
            uint16_t PADDR28:1;
            uint16_t PADDR27:1;
            uint16_t PADDR26:1;
            uint16_t PADDR25:1;
            uint16_t PADDR24:1;
            uint16_t PADDR23:1;
            uint16_t PADDR22:1;
            uint16_t PADDR21:1;
            uint16_t PADDR20:1;
            uint16_t PADDR19:1;
            uint16_t PADDR18:1;
            uint16_t PADDR17:1;
            uint16_t PADDR16:1;
            uint16_t PADDR15:1;
            uint16_t PADDR14:1;
            uint16_t PADDR13:1;
            uint16_t PADDR12:1;
            uint16_t PADDR11:1;
            uint16_t PADDR10:1;
            uint16_t PADDR9:1;
            uint16_t PADDR8:1;
            uint16_t PADDR7:1;
            uint16_t PADDR6:1;
            uint16_t PADDR5:1;
            uint16_t PADDR4:1;
            uint16_t PADDR3:1;
            uint16_t PADDR2:1;
            uint16_t PADDR1:1;
            uint16_t PADDR0:1;
        };
        struct {
            uint16_t u16[3];
        };
        struct {
            uint8_t u8[6];
        };
    } PADDR;
    union {
        struct {
            uint64_t u64;
        };
        struct {
            uint32_t u32[2];
        };
        struct {
            uint16_t u16[4];
        };
        struct {
            uint8_t u8[8];
        };
    } LADRF;
    union {
        struct {
            uint32_t RDRA15:1;
            uint32_t RDRA14:1;
            uint32_t RDRA13:1;
            uint32_t RDRA12:1;
            uint32_t RDRA11:1;
            uint32_t RDRA10:1;
            uint32_t RDRA9:1;
            uint32_t RDRA8:1;
            uint32_t RDRA7:1;
            uint32_t RDRA6:1;
            uint32_t RDRA5:1;
            uint32_t RDRA4:1;
            uint32_t RDRA3:1;
            uint32_t :3;
            uint32_t RLEN2:1;
            uint32_t RLEN1:1;
            uint32_t RLEN0:1;
            uint32_t :5;
            uint32_t RDRA23:1;
            uint32_t RDRA22:1;
            uint32_t RDRA21:1;
            uint32_t RDRA20:1;
            uint32_t RDRA19:1;
            uint32_t RDRA18:1;
            uint32_t RDRA17:1;
            uint32_t RDRA16:1;
        };
        struct {
            uint32_t RDRA15_3:13;
            uint32_t :3;
            uint32_t RLEN:3;
            uint32_t :5;
            uint32_t RDRA23_16:8;
        };
        struct {
            uint32_t u32;
        };
        struct {
            uint16_t u16[2];
        };
    } RDRP;
    union {
        struct {
            uint32_t TDRA15:1;
            uint32_t TDRA14:1;
            uint32_t TDRA13:1;
            uint32_t TDRA12:1;
            uint32_t TDRA11:1;
            uint32_t TDRA10:1;
            uint32_t TDRA9:1;
            uint32_t TDRA8:1;
            uint32_t TDRA7:1;
            uint32_t TDRA6:1;
            uint32_t TDRA5:1;
            uint32_t TDRA4:1;
            uint32_t TDRA3:1;
            uint32_t :3;
            uint32_t TLEN2:1;
            uint32_t TLEN1:1;
            uint32_t TLEN0:1;
            uint32_t :5;
            uint32_t TDRA23:1;
            uint32_t TDRA22:1;
            uint32_t TDRA21:1;
            uint32_t TDRA20:1;
            uint32_t TDRA19:1;
            uint32_t TDRA18:1;
            uint32_t TDRA17:1;
            uint32_t TDRA16:1;
        };
        struct {
            uint32_t TDRA15_3:13;
            uint32_t :3;
            uint32_t TLEN:3;
            uint32_t :5;
            uint32_t TDRA23_16:8;
        };
        struct {
            uint32_t u32;
        };
        struct {
            uint16_t u16[2];
        };
    } TDRP;
} LANCE_Init_Block_t;

typedef struct {
    union {
        struct {
            uint16_t LADR15:1;
            uint16_t LADR14:1;
            uint16_t LADR13:1;
            uint16_t LADR12:1;
            uint16_t LADR11:1;
            uint16_t LADR10:1;
            uint16_t LADR9:1;
            uint16_t LADR8:1;
            uint16_t LADR7:1;
            uint16_t LADR6:1;
            uint16_t LADR5:1;
            uint16_t LADR4:1;
            uint16_t LADR3:1;
            uint16_t LADR2:1;
            uint16_t LADR1:1;
            uint16_t LADR0:1;
        };
        struct {
            uint16_t LADR;
        };
        struct {
            uint16_t u16;
        };
    } RMD0;
    union {
        struct {
            uint16_t OWN:1;
            uint16_t ERR:1;
            uint16_t FRAM:1;
            uint16_t OFLO:1;
            uint16_t CRC:1;
            uint16_t BUFF:1;
            uint16_t STP:1;
            uint16_t ENP:1;
            uint16_t HADR23:1;
            uint16_t HADR22:1;
            uint16_t HADR21:1;
            uint16_t HADR20:1;
            uint16_t HADR19:1;
            uint16_t HADR18:1;
            uint16_t HADR17:1;
            uint16_t HADR16:1;
        };
        struct {
            uint16_t :8;
            uint16_t HADR:8;
        };
        struct {
            uint16_t u16;
        };
    } RMD1;
    union {
        struct {
            uint16_t ONES3:1;
            uint16_t ONES2:1;
            uint16_t ONES1:1;
            uint16_t ONES0:1;
            uint16_t BCNT11:1;
            uint16_t BCNT10:1;
            uint16_t BCNT9:1;
            uint16_t BCNT8:1;
            uint16_t BCNT7:1;
            uint16_t BCNT6:1;
            uint16_t BCNT5:1;
            uint16_t BCNT4:1;
            uint16_t BCNT3:1;
            uint16_t BCNT2:1;
            uint16_t BCNT1:1;
            uint16_t BCNT0:1;
        };
        struct {
            uint16_t ONES:4;
            uint16_t BCNT:12;
        };
        struct {
            uint16_t u16;
        };
    } RMD2;
    union {
        struct {
            uint16_t :4;
            uint16_t MCNT11:1;
            uint16_t MCNT10:1;
            uint16_t MCNT9:1;
            uint16_t MCNT8:1;
            uint16_t MCNT7:1;
            uint16_t MCNT6:1;
            uint16_t MCNT5:1;
            uint16_t MCNT4:1;
            uint16_t MCNT3:1;
            uint16_t MCNT2:1;
            uint16_t MCNT1:1;
            uint16_t MCNT0:1;
        };
        struct {
            uint16_t :4;
            uint16_t MCNT:12;
        };
        struct {
            uint16_t u16;
        };
    } RMD3;
} LANCE_Rx_Descriptor_t;

typedef struct {
    union {
        struct {
            uint16_t LADR15:1;
            uint16_t LADR14:1;
            uint16_t LADR13:1;
            uint16_t LADR12:1;
            uint16_t LADR11:1;
            uint16_t LADR10:1;
            uint16_t LADR9:1;
            uint16_t LADR8:1;
            uint16_t LADR7:1;
            uint16_t LADR6:1;
            uint16_t LADR5:1;
            uint16_t LADR4:1;
            uint16_t LADR3:1;
            uint16_t LADR2:1;
            uint16_t LADR1:1;
            uint16_t LADR0:1;
        };
        struct {
            uint16_t LADR;
        };
        struct {
            uint16_t u16;
        };
    } TMD0;
    union {
        struct {
            uint16_t OWN:1;
            uint16_t ERR:1;
            uint16_t ADD_FCS:1;
            uint16_t MORE:1;
            uint16_t ONE:1;
            uint16_t DEF:1;
            uint16_t STP:1;
            uint16_t ENP:1;
            uint16_t HADR23:1;
            uint16_t HADR22:1;
            uint16_t HADR21:1;
            uint16_t HADR20:1;
            uint16_t HADR19:1;
            uint16_t HADR18:1;
            uint16_t HADR17:1;
            uint16_t HADR16:1;
        };
        struct {
            uint16_t :8;
            uint16_t HADR:8;
        };
        struct {
            uint16_t u16;
        };
    } TMD1;
    union {
        struct {
            uint16_t ONES3:1;
            uint16_t ONES2:1;
            uint16_t ONES1:1;
            uint16_t ONES0:1;
            uint16_t BCNT11:1;
            uint16_t BCNT10:1;
            uint16_t BCNT9:1;
            uint16_t BCNT8:1;
            uint16_t BCNT7:1;
            uint16_t BCNT6:1;
            uint16_t BCNT5:1;
            uint16_t BCNT4:1;
            uint16_t BCNT3:1;
            uint16_t BCNT2:1;
            uint16_t BCNT1:1;
            uint16_t BCNT0:1;
        };
        struct {
            uint16_t ONES:4;
            uint16_t BCNT:12;
        };
        struct {
            uint16_t u16;
        };
    } TMD2;
    union {
        struct {
            uint16_t BUFF:1;
            uint16_t UFLO:1;
            uint16_t :1;
            uint16_t LCOL:1;
            uint16_t LCAR:1;
            uint16_t RTRY:1;
            uint16_t TDR9:1;
            uint16_t TDR8:1;
            uint16_t TDR7:1;
            uint16_t TDR6:1;
            uint16_t TDR5:1;
            uint16_t TDR4:1;
            uint16_t TDR3:1;
            uint16_t TDR2:1;
            uint16_t TDR1:1;
            uint16_t TDR0:1;
        };
        struct {
            uint16_t :6;
            uint16_t TDR:10;
        };
        struct {
            uint16_t u16;
        };
    } TMD3;
} LANCE_Tx_Descriptor_t;

/* FreeRTOS only has a single pool of buffers that is shared between TX and RX.
 * Therefore, and due to the available buffer count options provided by the
 * LANCE, an even power of 2 number of buffers must be configured. Then, half
 * of those buffers will be allocated during initialisation to the RX ring.
 * TODO: make the TX ring a bit smaller in future? */
#if ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS < 2
#error "Even number of buffers is required, and must be a power of 2"
#endif

#if ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS > 256
#error "LANCE supports a maximum of 128 buffers per ring, therefore ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS must be <= 256"
#endif

/* TX_RING_SZ and RX_RING_SZ are the number of entries in the descriptor rings */
#define TX_RING_SZ (ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS / 2)
#define RX_RING_SZ (ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS / 2)

/* Define ring lengths here, makes VSCode more happy */
#define RX_RING_LEN 0
#define TX_RING_LEN 0

#if TX_RING_SZ == 1
/* Already defined, so ignore */
#elif TX_RING_SZ == 2
#undef TX_RING_LEN
#define TX_RING_LEN 1
#elif TX_RING_SZ == 4
#undef TX_RING_LEN
#define TX_RING_LEN 2
#elif TX_RING_SZ == 8
#undef TX_RING_LEN
#define TX_RING_LEN 3
#elif TX_RING_SZ == 16
#undef TX_RING_LEN
#define TX_RING_LEN 4
#elif TX_RING_SZ == 32
#undef TX_RING_LEN
#define TX_RING_LEN 5
#elif TX_RING_SZ == 64
#undef TX_RING_LEN
#define TX_RING_LEN 6
#elif TX_RING_SZ == 128
#undef TX_RING_LEN
#define TX_RING_LEN 7
#else
#error "Invalid TX ring size"
#endif

#if RX_RING_SZ == 1
/* Already defined, so ignore */
#elif RX_RING_SZ == 2
#undef RX_RING_LEN
#define RX_RING_LEN 1
#elif RX_RING_SZ == 4
#undef RX_RING_LEN
#define RX_RING_LEN 2
#elif RX_RING_SZ == 8
#undef RX_RING_LEN
#define RX_RING_LEN 3
#elif RX_RING_SZ == 16
#undef RX_RING_LEN
#define RX_RING_LEN 4
#elif RX_RING_SZ == 32
#undef RX_RING_LEN
#define RX_RING_LEN 5
#elif RX_RING_SZ == 64
#undef RX_RING_LEN
#define RX_RING_LEN 6
#elif RX_RING_SZ == 128
#undef RX_RING_LEN
#define RX_RING_LEN 7
#else
#error "Invalid RX ring size"
#endif

/* Reserve some space statically to hold the Initialisation Block */
uint16_t LANCE_Init_Block[12] __attribute__((section(".iomem")));

/* Reserve space for the transmit descriptor ring, which must be aligned to
 * 8 bytes. */
LANCE_Tx_Descriptor_t LANCE_Tx_Descriptor_Ring[TX_RING_SZ] __attribute__((aligned(8), section(".iomem")));

/* Reserve space for the receive descriptor ring. Sizing/alignment is the same
 * as the transmit descriptor ring. */
LANCE_Rx_Descriptor_t LANCE_Rx_Descriptor_Ring[RX_RING_SZ] __attribute__((aligned(8), section(".iomem")));

/* Reserve space for TX/RX buffers. One pool is used to manage both, and is
 * divided in two, with one half initially being assigned to the RX ring. The
 * other half is thus available for the TX ring as required. Although the LANCE
 * doesnt have any special alignment requirements, FreeRTOS likes to have a
 * pointer to the descriptor that the buffer belongs to stored before the
 * actual buffer itself, and that pointer will need to be word aligned.
 * Therefore, ensure that each buffer is an even number of bytes long, and tell
 * the compiler to align the buffer allocation itself to a word boundary. */
#define BUFFER_SIZE (ipTOTAL_ETHERNET_FRAME_SIZE + ipBUFFER_PADDING)
#define BUFFER_SIZE_ROUNDED_UP ((BUFFER_SIZE + 1) & ~0x1UL)
uint8_t LANCE_Buffers[ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS][BUFFER_SIZE_ROUNDED_UP] __attribute__((aligned(2), section(".iomem")));

/* Transmit tail and head indexes, referencing the index of the next available
 * transmit descriptor (head), and the index of the currently transmitting
 * descriptor (tail) */
static uint8_t tx_tail_idx = 0;
static uint8_t tx_head_idx = 0;

/* Stores the value of xReleaseAfterSend that was provided to
 * xNetworkInterfaceOutput(), and is used on completion of packet TX to
 * determine whether the buffers should be released or not. */
static BaseType_t relase_after_sends[TX_RING_SZ] = {0};






/* Release all descriptors that have been allocated to the RX ring */
static void
release_allocated_descriptors(uint8_t numbufs)
{
    LANCE_Rx_Descriptor_t *rdrp = &(LANCE_Rx_Descriptor_Ring[0]);
    uint8_t index;
    uint8_t *ring_buf;
    NetworkBufferDescriptor_t *buf;

    for (index = 0; index < numbufs; index++) {
        ring_buf = (uint8_t *)(rdrp[index].RMD1.HADR << 16 | rdrp[index].RMD0.LADR);

        buf = pxPacketBuffer_to_NetworkBuffer((void *)ring_buf);

        if (buf != NULL) {
            vReleaseNetworkBufferAndDescriptor(buf);
        }
    }
}

BaseType_t
xNetworkInterfaceInitialise(void)
{
    LANCE_Init_Block_t *ib;
    LANCE_Rx_Descriptor_t *rdrp;
    LANCE_Tx_Descriptor_t *tdrp;
    uint8_t index;
    uint16_t ctr;
    uint16_t delay;
    NetworkBufferDescriptor_t *buf;
    uint8_t numbufs = 0;
    const uint16_t *mac_addr;

    /* Set up pointers to initialisation block, buffers etc */
    ib = (LANCE_Init_Block_t *)&LANCE_Init_Block;
    tdrp = (LANCE_Tx_Descriptor_t *)&LANCE_Tx_Descriptor_Ring;
    rdrp = (LANCE_Rx_Descriptor_t *)&LANCE_Rx_Descriptor_Ring;

    /* Setup the initialisation block */
    ib->MODE.u16 = 0;               /* Mode all zeroes */

    /* MAC address */
    mac_addr = (const uint16_t *)FreeRTOS_GetMACAddress();

    ib->PADDR.u16[0] = __builtin_bswap16(mac_addr[0]);
    ib->PADDR.u16[1] = __builtin_bswap16(mac_addr[1]);
    ib->PADDR.u16[2] = __builtin_bswap16(mac_addr[2]);

    /* Ignore all multicast packets */
    ib->LADRF.u64 = 0;

    /* Receive descriptor ring pointer, and descriptor ring size */
    ib->RDRP.u16[0] = (uint32_t)rdrp;
    ib->RDRP.u16[1] = (uint32_t)rdrp >> 16;
    ib->RDRP.RLEN = RX_RING_LEN;

    /* Transmit descriptor ring pointer, and descriptor ring size */
    ib->TDRP.u16[0] = (uint32_t)tdrp;
    ib->TDRP.u16[1] = (uint32_t)tdrp >> 16;
    ib->TDRP.TLEN = TX_RING_LEN;

    /* Initialise RX descriptor ring - each buffer initially is owned by the
     * LANCE. */
    for (index = 0; index < RX_RING_SZ; index++) {
        /* Get a buffer to assign to descriptor */
        buf = pxGetNetworkBufferWithDescriptor(BUFFER_SIZE_ROUNDED_UP, 0);

        if (buf == NULL) {
            /* If buffer allocation fails, network has not initialised. Break
             * and free any buffers that were allocated, then fail. */
            break;
        }

        numbufs++;

        /* Lower 16 bits of buffer address */
        rdrp[index].RMD0.LADR = (uint32_t)buf->pucEthernetBuffer;

        /* MSb to 1 so LANCE owns the descriptor, and upper 8 bits of buffer
         * address */
        rdrp[index].RMD1.u16 = 0x8000;
        rdrp[index].RMD1.HADR = (uint32_t)buf->pucEthernetBuffer >> 16;

        /* Upper nibble must be all ones, set the size of the buffer */
        rdrp[index].RMD2.u16 = 0xF000;
        rdrp[index].RMD2.BCNT = BUFFER_SIZE_ROUNDED_UP;

        rdrp[index].RMD3.u16 = 0;
    }

    /* Initialise TX descriptor ring - each buffer initially is owned by the
     * host, and has a null pointer which will be updated to point to a buffer
     * when a frame needs to be transmitted. */
    for (index = 0; index < TX_RING_SZ; index++) {
        /* Lower 16 bits of buffer address - fill with index value for debug */
        tdrp[index].TMD0.u16 = 0;
        
        /* MSb to 0 so host owns the descriptor, and upper 8 bits of buffer
         * address */
        tdrp[index].TMD1.u16 = 0;
        
        /* Upper nibble must be all ones */
        tdrp[index].TMD2.u16 = 0xF000;

        tdrp[index].TMD3.u16 = 0;
    }

    if (numbufs < RX_RING_SZ) {
        /* Not all buffers were allocated, which means that initialisation
         * failed. Release the buffers that were allocated and then return
         * false to fail the initialisation process. */
        release_allocated_descriptors(numbufs);

        return pdFALSE;
    }

    /* Set STOP bit of CSR0 to reset and ensure LANCE is ready for init */
    LANCE_RAPbits.CSR = 0;
    LANCE_CSR0bits.STOP = 1;

    /* Set the address of the IB in the LANCE CSRs and tell it to initialise */
    LANCE_RAPbits.CSR = 1;
    LANCE_CSR1 = (uint32_t)ib;
    LANCE_RAPbits.CSR = 2;
    LANCE_CSR2 = (uint32_t)ib >> 16;

    /* Set BSWP bit because m68k is big endian, and bits 15..8 fall on even
     * addresses */
    LANCE_RAPbits.CSR = 3;
    LANCE_CSR3bits.u16 = 0x4;

    /* Tell the LANCE to initialise. Leave the RAP pointing at CSR0 as this is
     * where all of the status flags are located */
    LANCE_RAPbits.CSR = 0;
    LANCE_CSR0 = 1;             /* Write a literal 1 to prevent the STOP bit
                                 * being re-set */

    /* Check if initialisation succeeded */
    for (ctr = 100; ctr > 0; ctr--) {
        for (delay = 1000; delay > 0; delay--);

        if (LANCE_CSR0bits.IDON) {
            LANCE_CSR0bits.IDON = 1;    /* Clear IDON flag */
            LANCE_CSR0bits.INEA = 1;    /* Enable interrupts */
            
            break;
        }
    }

    if (ctr == 0) {
        /* Initialisation failed */
        release_allocated_descriptors(numbufs);

        return pdFALSE;
    }

    /* Start only after successful init, wait for transmitter and receiver to
     * be enabled */
    LANCE_CSR0bits.STRT = 1;

    while ((LANCE_CSR0 & 0x0030) != 0x0030);

    /* Done, return success */
    return pdTRUE;
}

void vNetworkInterfaceAllocateRAMToBuffers(
    NetworkBufferDescriptor_t pxNetworkBuffers[ ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS ] )
{
    uint32_t index;

    for (index = 0; index < ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS; index++) {
        pxNetworkBuffers[index].pucEthernetBuffer = &(LANCE_Buffers[index][ipBUFFER_PADDING]);
        *((uint32_t *)&LANCE_Buffers[index][0]) = (uint32_t)(&(pxNetworkBuffers[index]));
    }
}













BaseType_t xNetworkInterfaceOutput( NetworkBufferDescriptor_t * const pxDescriptor,
                                    BaseType_t xReleaseAfterSend )
{
    LANCE_Tx_Descriptor_t *td;

    printf("pxDescriptor %p\r\n", pxDescriptor);

    /* Check that the head index is not owned by the LANCE */
    if (LANCE_Tx_Descriptor_Ring[tx_head_idx].TMD1.OWN == 1) {
        /* If the head index is owned by the LANCE, all TX descriptors are in
         * use */
        /* TODO: release buffer and fail */
        return pdFALSE;
    }

    /* Store xReleaseAfterSend for later */
    relase_after_sends[tx_head_idx] = xReleaseAfterSend;

    /* Assign the td pointer */
    td = &LANCE_Tx_Descriptor_Ring[tx_head_idx];

    /* Clear all flags for this descriptor */
    td->TMD1.u16 = 0;

    /* Put pointer to buffer into descriptor */
    td->TMD0.LADR = (uint32_t)pxDescriptor->pucEthernetBuffer;
    td->TMD1.HADR = (uint32_t)pxDescriptor->pucEthernetBuffer >> 16;

    /* Put buffer length into descriptor */
    td->TMD2.BCNT = -(pxDescriptor->xDataLength);

    printf("pucEthernetBuffer %p\r\n", pxDescriptor->pucEthernetBuffer);
    printf("xDataLength %d\r\n", pxDescriptor->xDataLength);

    /* Set descriptor flags */
    td->TMD1.STP = 1;                   /* The first buffer of this packet */
    td->TMD1.ENP = 1;                   /* ... and also the last */
    td->TMD1.ADD_FCS = 1;               /* Hardware adds checksum */
    td->TMD1.OWN = 1;                   /* LANCE will now TX */

    /* Begin: Dump TD for debug */
    uint16_t *ptr = (uint16_t *)td;
    uint16_t ctr;

    printf("TD> (%p): ", td);

    for (ctr = 0; ctr < 4; ctr++) {
        printf("%04X ", *ptr);
        ptr++;
    }

    printf("\r\n");

    uint8_t *c = pxDescriptor->pucEthernetBuffer;

    for (ctr = 0; ctr < 64; ctr++) {
        printf("%02X ", *c++);
    }

    printf("\r\n");
    /* End: Dump TD for debug */

    /* Advance the head pointer */
    tx_head_idx++;

    if (tx_head_idx == TX_RING_SZ) {
        /* Wrap the head pointer at the last descriptor in the arrary */
        tx_head_idx = 0;
    }

    return pdTRUE;
}


static void
LANCE_TX_Complete(void)
{
    LANCE_Tx_Descriptor_t *td;
    uint8_t *buf;
    NetworkBufferDescriptor_t *pxDescriptor;

    /* TODO: more error handling is likely required */

    /* Assign the td pointer */
    td = &LANCE_Tx_Descriptor_Ring[tx_tail_idx];

    /* Release the buffer if required */
    if (relase_after_sends[tx_tail_idx] != pdFALSE) {
        buf = (uint8_t *)(td->TMD1.HADR << 16 | td->TMD0.LADR);

        pxDescriptor = pxPacketBuffer_to_NetworkBuffer((void *)buf);

        // TODO: seems to be crashing here...
        // vReleaseNetworkBufferAndDescriptor(pxDescriptor);
    }

    /* Advance the tail pointer */
    tx_tail_idx++;

    if (tx_tail_idx == TX_RING_SZ) {
        /* Wrap the head pointer at the last descriptor in the arrary */
        tx_tail_idx = 0;
    }
}











void
LANCE_Interrupt_Handler(void)
{
    /* Write value of CSR0 to display board for debugging */
    *(uint32_t *)(0x03000000) = (uint32_t)LANCE_CSR0;
    // printf("%04X -> ", LANCE_CSR0);

    if (LANCE_CSR0bits.BABL) {
        /* Handle BABLE event */

        LANCE_CSR0bits.BABL = 1;    /* Reset event flag */
    }

    if (LANCE_CSR0bits.MISS) {
        /* Handle MISSED PACKET event */

        LANCE_CSR0bits.MISS = 1;    /* Reset event flag */
    }

    if (LANCE_CSR0bits.MERR) {
        /* Handle MEMORY ERROR event */

        LANCE_CSR0bits.MERR = 1;    /* Reset event flag */
    }

    if (LANCE_CSR0bits.RINT) {
        /* Handle RECEIVER INTERRUPT event */

        LANCE_CSR0bits.RINT = 1;    /* Reset event flag */
    }

    if (LANCE_CSR0bits.TINT) {
        /* Handle TRANSMITTER INTERRUPT event */
        LANCE_TX_Complete();

        LANCE_CSR0bits.TINT = 1;    /* Reset event flag */
    }

    // printf("%04X\r\n", LANCE_CSR0);
}