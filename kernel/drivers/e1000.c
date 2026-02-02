/* Intel e1000: PCI probe, BAR0, TX/RX rings, basic send/recv */
#include "kernel/drivers/e1000.h"
#include "kernel/drivers/pci.h"
#include "kernel/mem/pmm.h"
#include "kernel/mem/vmm.h"
#include "lib/string.h"

#define E1000_VENDOR 0x8086
#define E1000_DEVICE 0x100E  /* 82540EM */

#define E1000_CTRL    0x0000
#define E1000_STATUS  0x0008
#define E1000_EERD    0x0014
#define E1000_FCAL    0x0028
#define E1000_FCAH    0x0030
#define E1000_FCT     0x0038
#define E1000_VET     0x0038
#define E1000_ICR     0x00C0
#define E1000_ITR     0x00C4
#define E1000_ICS     0x00C8
#define E1000_IMS     0x00D0
#define E1000_IMC     0x00D8
#define E1000_RCTL    0x0100
#define E1000_TCTL    0x0400
#define E1000_TIPG    0x0410
#define E1000_TDBAL   0x3800
#define E1000_TDBAH   0x3804
#define E1000_TDLEN   0x3808
#define E1000_TDH     0x3810
#define E1000_TDT     0x3818
#define E1000_RDBAL   0x2800
#define E1000_RDBAH   0x2804
#define E1000_RDLEN   0x2808
#define E1000_RDH     0x2810
#define E1000_RDT     0x2818
#define E1000_RAL     0x5400
#define E1000_RAH     0x5404

#define RCTL_EN       (1<<1)
#define RCTL_BAM      (1<<15)
#define TCTL_EN       (1<<1)
#define TCTL_PSP      (1<<3)
#define CMD_ASDE      (1<<5)
#define CTRL_RST      (1<<26)

#define RDESC_SIZE 16
#define TDESC_SIZE 16
#define NUM_RX 32
#define NUM_TX 8
#define BUF_SIZE 1536

static volatile uint32_t *e1000_bar0;
static uint8_t e1000_bus, e1000_dev, e1000_func;
static void *rx_bufs[NUM_RX];
static void *tx_bufs[NUM_TX];
static uint64_t rx_descs_phys, tx_descs_phys;
static void *rx_descs_virt, *tx_descs_virt;
static uint32_t rx_tail, tx_head, tx_tail;

static inline uint32_t read_reg(uint32_t reg) { return e1000_bar0[reg / 4]; }
static inline void write_reg(uint32_t reg, uint32_t val) { e1000_bar0[reg / 4] = val; }

void e1000_init(void)
{
    e1000_bar0 = NULL;  /* Initialize to NULL */
    if (!pci_find(E1000_VENDOR, E1000_DEVICE, &e1000_bus, &e1000_dev, &e1000_func)) {
        return;
    }
    uint32_t bar0 = pci_read(e1000_bus, e1000_dev, e1000_func, 0x10);
    if (bar0 == 0 || bar0 == 0xFFFFFFFF) {
        return;  /* Invalid BAR0 */
    }
    pci_write(e1000_bus, e1000_dev, e1000_func, 0x10, 0xFFFFFFFF);
    uint32_t size = pci_read(e1000_bus, e1000_dev, e1000_func, 0x10);
    pci_write(e1000_bus, e1000_dev, e1000_func, 0x10, bar0);
    size &= ~0xF;
    size = ~size + 1;
    (void)size;
    uintptr_t phys = bar0 & ~0xF;
    if (phys == 0) {
        return;  /* Invalid physical address */
    }
    e1000_bar0 = (volatile uint32_t *)vmm_map_io(phys);
    if (!e1000_bar0) {
        return;  /* vmm_map_io failed */
    }

    write_reg(E1000_CTRL, read_reg(E1000_CTRL) | CTRL_RST);
    while (read_reg(E1000_CTRL) & CTRL_RST);

    void *rx_page = pmm_alloc_page();
    void *tx_page = pmm_alloc_page();
    rx_descs_phys = (uintptr_t)rx_page;
    tx_descs_phys = (uintptr_t)tx_page;
    if (!rx_page || !tx_page) return;
    rx_descs_virt = vmm_map_io(rx_descs_phys);
    tx_descs_virt = vmm_map_io(tx_descs_phys);
    for (int i = 0; i < NUM_RX; i++) {
        rx_bufs[i] = pmm_alloc_page();
        if (!rx_bufs[i]) return;
    }
    for (int i = 0; i < NUM_TX; i++) {
        tx_bufs[i] = pmm_alloc_page();
        if (!tx_bufs[i]) return;
    }

    struct rx_desc { uint64_t addr; uint16_t length; uint16_t csum; uint8_t status; uint8_t errors; uint16_t vlan; };
    struct tx_desc { uint64_t addr; uint16_t length; uint8_t cso; uint8_t cmd; uint8_t status; uint8_t css; uint16_t vlan; };
    struct rx_desc *rx = (struct rx_desc *)rx_descs_virt;
    struct tx_desc *tx = (struct tx_desc *)tx_descs_virt;
    for (int i = 0; i < NUM_RX; i++) {
        rx[i].addr = (uintptr_t)rx_bufs[i];
        rx[i].length = 0;
        rx[i].csum = 0;
        rx[i].status = 0;
    }
    for (int i = 0; i < NUM_TX; i++) {
        tx[i].addr = 0;
        tx[i].length = 0;
        tx[i].cmd = 0;
        tx[i].status = 0;
    }

    write_reg(E1000_RDBAL, (uint32_t)rx_descs_phys);
    write_reg(E1000_RDBAH, (uint32_t)(rx_descs_phys >> 32));
    write_reg(E1000_RDLEN, NUM_RX * RDESC_SIZE);
    write_reg(E1000_RDH, 0);
    write_reg(E1000_RDT, NUM_RX - 1);
    write_reg(E1000_RCTL, RCTL_EN | RCTL_BAM);

    write_reg(E1000_TDBAL, (uint32_t)tx_descs_phys);
    write_reg(E1000_TDBAH, (uint32_t)(tx_descs_phys >> 32));
    write_reg(E1000_TDLEN, NUM_TX * TDESC_SIZE);
    write_reg(E1000_TDH, 0);
    write_reg(E1000_TDT, 0);
    write_reg(E1000_TCTL, TCTL_EN | TCTL_PSP);
    write_reg(E1000_TIPG, 0x0060200A);

    rx_tail = NUM_RX - 1;
    tx_head = tx_tail = 0;
}

bool e1000_send(const void *buf, size_t len)
{
    if (!e1000_bar0 || len > BUF_SIZE) return false;
    struct tx_desc { uint64_t addr; uint16_t length; uint8_t cso; uint8_t cmd; uint8_t status; uint8_t css; uint16_t vlan; };
    struct tx_desc *tx = (struct tx_desc *)tx_descs_virt;
    uint32_t next = (tx_tail + 1) % NUM_TX;
    if (next == tx_head) return false;
    memcpy(tx_bufs[tx_tail], buf, len);
    tx[tx_tail].addr = (uintptr_t)tx_bufs[tx_tail];
    tx[tx_tail].length = (uint16_t)len;
    tx[tx_tail].cmd = 3;  /* EOP + IFCS */
    tx[tx_tail].status = 0;
    write_reg(E1000_TDT, next);
    tx_tail = next;
    return true;
}

int e1000_recv(void *buf, size_t max_len)
{
    if (!e1000_bar0) return -1;
    struct rx_desc { uint64_t addr; uint16_t length; uint16_t csum; uint8_t status; uint8_t errors; uint16_t vlan; };
    struct rx_desc *rx = (struct rx_desc *)rx_descs_virt;
    uint32_t next = (rx_tail + 1) % NUM_RX;
    if (!(rx[next].status & 1)) return 0;
    size_t len = rx[next].length;
    if (len > max_len) len = max_len;
    memcpy(buf, rx_bufs[next], len);
    rx[next].status = 0;
    write_reg(E1000_RDT, rx_tail = next);
    return (int)len;
}
