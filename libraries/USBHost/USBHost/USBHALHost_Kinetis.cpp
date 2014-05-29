/* mbed USBHost Library
 * Copyright (c) 2006-2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#if defined(TARGET_KL46Z)

#include "mbed.h"
#include "USBHALHost.h"
#include "dbg.h"

#define BD_OWN_MASK        (1 << 7)
#define BD_DATA01_MASK     (1 << 6)
#define BD_KEEP_MASK       (1 << 5)
#define BD_NINC_MASK       (1 << 4)
#define BD_DTS_MASK        (1 << 3)
#define BD_STALL_MASK      (1 << 2)

#define TX    1
#define RX    0

#define EP0_BDT_IDX(dir, odd) (((2 * dir) + (1 * odd)))

#define SETUP_TOKEN    0x0D
#define IN_TOKEN       0x09
#define OUT_TOKEN      0x01

/* buffer descriptor table */
__attribute__((__aligned__(512))) BDT bdt[64];

// for each endpt: 8 bytes
struct BDT {
    uint8_t   info;       // BD[0:7]
    uint8_t   dummy;      // RSVD: BD[8:15]
    uint16_t  byte_count; // BD[16:32]
    uint32_t  address;    // Addr
};

USBHALHost * USBHALHost::instHost;

USBHALHost::USBHALHost() {
    instHost = this;
}

void USBHALHost::init() {

}

uint32_t USBHALHost::controlHeadED() {
    return LPC_USB->HcControlHeadED;
}

uint32_t USBHALHost::bulkHeadED() {
    return LPC_USB->HcBulkHeadED;
}

uint32_t USBHALHost::interruptHeadED() {
    return usb_hcca->IntTable[0];
}

void USBHALHost::updateBulkHeadED(uint32_t addr) {
    LPC_USB->HcBulkHeadED = addr;
}


void USBHALHost::updateControlHeadED(uint32_t addr) {
    LPC_USB->HcControlHeadED = addr;
}

void USBHALHost::updateInterruptHeadED(uint32_t addr) {
    usb_hcca->IntTable[0] = addr;
}


void USBHALHost::enableList(ENDPOINT_TYPE type) {
    switch(type) {
        case CONTROL_ENDPOINT:
            LPC_USB->HcCommandStatus = OR_CMD_STATUS_CLF;
            LPC_USB->HcControl |= OR_CONTROL_CLE;
            break;
        case ISOCHRONOUS_ENDPOINT:
            break;
        case BULK_ENDPOINT:
            LPC_USB->HcCommandStatus = OR_CMD_STATUS_BLF;
            LPC_USB->HcControl |= OR_CONTROL_BLE;
            break;
        case INTERRUPT_ENDPOINT:
            LPC_USB->HcControl |= OR_CONTROL_PLE;
            break;
    }
}


bool USBHALHost::disableList(ENDPOINT_TYPE type) {
    switch(type) {
        case CONTROL_ENDPOINT:
            if(LPC_USB->HcControl & OR_CONTROL_CLE) {
                LPC_USB->HcControl &= ~OR_CONTROL_CLE;
                return true;
            }
            return false;
        case ISOCHRONOUS_ENDPOINT:
            return false;
        case BULK_ENDPOINT:
            if(LPC_USB->HcControl & OR_CONTROL_BLE){
                LPC_USB->HcControl &= ~OR_CONTROL_BLE;
                return true;
            }
            return false;
        case INTERRUPT_ENDPOINT:
            if(LPC_USB->HcControl & OR_CONTROL_PLE) {
                LPC_USB->HcControl &= ~OR_CONTROL_PLE;
                return true;
            }
            return false;
    }
    return false;
}


void USBHALHost::memInit() {
    usb_hcca = (volatile HCCA *)usb_buf;
    usb_edBuf = usb_buf + HCCA_SIZE;
    usb_tdBuf = usb_buf + HCCA_SIZE + (MAX_ENDPOINT*ED_SIZE);
}

volatile uint8_t * USBHALHost::getED() {
    for (int i = 0; i < MAX_ENDPOINT; i++) {
        if ( !edBufAlloc[i] ) {
            edBufAlloc[i] = true;
            return (volatile uint8_t *)(usb_edBuf + i*ED_SIZE);
        }
    }
    perror("Could not allocate ED\r\n");
    return NULL; //Could not alloc ED
}

volatile uint8_t * USBHALHost::getTD() {
    int i;
    for (i = 0; i < MAX_TD; i++) {
        if ( !tdBufAlloc[i] ) {
            tdBufAlloc[i] = true;
            return (volatile uint8_t *)(usb_tdBuf + i*TD_SIZE);
        }
    }
    perror("Could not allocate TD\r\n");
    return NULL; //Could not alloc TD
}


void USBHALHost::freeED(volatile uint8_t * ed) {
    int i;
    i = (ed - usb_edBuf) / ED_SIZE;
    edBufAlloc[i] = false;
}

void USBHALHost::freeTD(volatile uint8_t * td) {
    int i;
    i = (td - usb_tdBuf) / TD_SIZE;
    tdBufAlloc[i] = false;
}


void USBHALHost::resetRootHub() {
    // Initiate port reset
    LPC_USB->HcRhPortStatus1 = OR_RH_PORT_PRS;

    while (LPC_USB->HcRhPortStatus1 & OR_RH_PORT_PRS);

    // ...and clear port reset signal
    LPC_USB->HcRhPortStatus1 = OR_RH_PORT_PRSC;
}


void USBHALHost::_usbisr(void) {
    if (instHost) {
        instHost->UsbIrqhandler();
    }
}

void USBHALHost::UsbIrqhandler() {
    if( LPC_USB->HcInterruptStatus & LPC_USB->HcInterruptEnable ) //Is there something to actually process?
    {

        uint32_t int_status = LPC_USB->HcInterruptStatus & LPC_USB->HcInterruptEnable;

        // Root hub status change interrupt
        if (int_status & OR_INTR_STATUS_RHSC) {
            if (LPC_USB->HcRhPortStatus1 & OR_RH_PORT_CSC) {
                if (LPC_USB->HcRhStatus & OR_RH_STATUS_DRWE) {
                    // When DRWE is on, Connect Status Change
                    // means a remote wakeup event.
                } else {

                    //Root device connected
                    if (LPC_USB->HcRhPortStatus1 & OR_RH_PORT_CCS) {

                        // wait 150ms to avoid bounce
                        wait_ms(150);

                        //Hub 0 (root hub), Port 1 (count starts at 1), Low or High speed
                        deviceConnected(0, 1, LPC_USB->HcRhPortStatus1 & OR_RH_PORT_LSDA);
                    }

                    //Root device disconnected
                    else {

                        if (!(int_status & OR_INTR_STATUS_WDH)) {
                            usb_hcca->DoneHead = 0;
                        }

                        // wait 200ms to avoid bounce
                        wait_ms(200);

                        deviceDisconnected(0, 1, NULL, usb_hcca->DoneHead & 0xFFFFFFFE);

                        if (int_status & OR_INTR_STATUS_WDH) {
                            usb_hcca->DoneHead = 0;
                            LPC_USB->HcInterruptStatus = OR_INTR_STATUS_WDH;
                        }
                    }
                }
                LPC_USB->HcRhPortStatus1 = OR_RH_PORT_CSC;
            }
            if (LPC_USB->HcRhPortStatus1 & OR_RH_PORT_PRSC) {
                LPC_USB->HcRhPortStatus1 = OR_RH_PORT_PRSC;
            }
            LPC_USB->HcInterruptStatus = OR_INTR_STATUS_RHSC;
        }

        // Writeback Done Head interrupt
        if (int_status & OR_INTR_STATUS_WDH) {
            transferCompleted(usb_hcca->DoneHead & 0xFFFFFFFE);
            LPC_USB->HcInterruptStatus = OR_INTR_STATUS_WDH;
        }
    }
}

#endif
