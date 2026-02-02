#include "kernel/drivers/pci.h"

#define PCI_CONFIG 0xCF8
#define PCI_DATA   0xCFC

static inline uint32_t pci_config_addr(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg)
{
    return 0x80000000u | (uint32_t)bus << 16 | (uint32_t)dev << 11 | (uint32_t)func << 8 | (reg & 0xFC);
}

static inline void outl(uint16_t port, uint32_t v)
{
    __asm__ volatile ("outl %0, %1" : : "a"(v), "Nd"(port));
}
static inline uint32_t inl(uint16_t port)
{
    uint32_t v;
    __asm__ volatile ("inl %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

uint32_t pci_read(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg)
{
    outl(PCI_CONFIG, pci_config_addr(bus, dev, func, reg));
    return inl(PCI_DATA);
}

void pci_write(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg, uint32_t val)
{
    outl(PCI_CONFIG, pci_config_addr(bus, dev, func, reg));
    outl(PCI_DATA, val);
}

bool pci_find(uint16_t vendor, uint16_t device, uint8_t *bus, uint8_t *dev, uint8_t *func)
{
    for (unsigned b = 0; b < 256; b++)
        for (uint8_t d = 0; d < 32; d++)
            for (uint8_t f = 0; f < 8; f++) {
                uint32_t id = pci_read((uint8_t)b, d, f, 0);
                if ((id & 0xFFFF) == vendor && (id >> 16) == device) {
                    *bus = (uint8_t)b; *dev = d; *func = f;
                    return true;
                }
            }
    return false;
}
