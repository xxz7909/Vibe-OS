#ifndef PCI_H
#define PCI_H
#include <stdint.h>
#include <stdbool.h>

uint32_t pci_read(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg);
void pci_write(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg, uint32_t val);
bool pci_find(uint16_t vendor, uint16_t device, uint8_t *bus, uint8_t *dev, uint8_t *func);

#endif
