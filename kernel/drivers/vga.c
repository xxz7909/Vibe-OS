#include <stdint.h>

#define VGA_BASE    0xB8000
#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_ATTR    0x07

static volatile uint16_t *vga = (volatile uint16_t *)VGA_BASE;
static int vga_row, vga_col;

static inline void outb(uint16_t port, uint8_t v)
{
    __asm__ volatile ("outb %0, %1" : : "a"(v), "Nd"(port));
}

static void vga_update_cursor(void)
{
    uint16_t pos = (uint16_t)(vga_row * VGA_WIDTH + vga_col);
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void vga_clear(void)
{
    vga_row = vga_col = 0;
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        vga[i] = (uint16_t)(VGA_ATTR << 8 | ' ');
    vga_update_cursor();
}

void vga_putchar_at(int row, int col, char c)
{
    if (row >= 0 && row < VGA_HEIGHT && col >= 0 && col < VGA_WIDTH)
        vga[row * VGA_WIDTH + col] = (uint16_t)(VGA_ATTR << 8 | (unsigned char)c);
}

static void scroll(void)
{
    for (int r = 0; r < VGA_HEIGHT - 1; r++)
        for (int c = 0; c < VGA_WIDTH; c++)
            vga[r * VGA_WIDTH + c] = vga[(r + 1) * VGA_WIDTH + c];
    for (int c = 0; c < VGA_WIDTH; c++)
        vga[(VGA_HEIGHT - 1) * VGA_WIDTH + c] = (uint16_t)(VGA_ATTR << 8 | ' ');
    vga_row = VGA_HEIGHT - 1;
    vga_col = 0;
    vga_update_cursor();
}

void vga_puts(const char *s)
{
    while (*s) {
        if (*s == '\n' || *s == '\r') {
            vga_col = 0;
            if (*s == '\n') {
                vga_row++;
                if (vga_row >= VGA_HEIGHT) scroll();
            }
        } else {
            /* Write directly to VGA memory */
            if (vga_row < VGA_HEIGHT && vga_col < VGA_WIDTH) {
                vga[vga_row * VGA_WIDTH + vga_col] = (uint16_t)(VGA_ATTR << 8 | (unsigned char)*s);
            }
            vga_col++;
            if (vga_col >= VGA_WIDTH) {
                vga_col = 0;
                vga_row++;
                if (vga_row >= VGA_HEIGHT) scroll();
            }
        }
        s++;
    }
    vga_update_cursor();
}
