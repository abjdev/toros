#include <drivers/devices/ps2/keyboard/kbscancodetable.hpp>
#include <drivers/devices/ps2/keyboard/keyboard.hpp>
#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <system/sched/lock/lock.hpp>
#include <system/cpu/idt/idt.hpp>
#include <lib/string.hpp>
#include <lib/memory.hpp>
#include <lib/io.hpp>

using namespace kernel::drivers::display;
using namespace kernel::system::cpu;

namespace kernel::drivers::ps2::kbd {

bool initialised = false;

char retstr[1024] = "\0";
bool reading = false;
int gi = 0;

volatile bool pressed = false;
volatile bool enter = false;

kbd_mod_t kbd_mod;

bool ps2_wait_write()
{
    int timer = 500;
    while ((inb(0x64) & 2) && timer-- > 0) asm volatile ("pause");
    return timer != 0;
}

bool ps2_write(uint32_t port, uint8_t b)
{
    if (ps2_wait_write())
    {
        outb(port, b);
        return true;
    }
    return false;
}

bool ps2_wait_read()
{
    int timer = 500;
    while ((inb(0x64) & 1) && timer-- > 0) asm volatile ("pause");
    return timer != 0;
}

uint8_t ps2_read(uint32_t port)
{
    if (ps2_wait_write()) return inb(port);
    return -1;
}

// Scancode to ascii
char get_ascii_char(uint8_t key_code)
{
    if (!kbd_mod.shift && !kbd_mod.capslock)
    {
        return kbdus[key_code];
    }
    if (kbd_mod.shift && !kbd_mod.capslock)
    {
        return kbdus_shft[key_code];
    }
    if (!kbd_mod.shift && kbd_mod.capslock)
    {
        return kbdus_caps[key_code];
    }
    if (kbd_mod.shift && kbd_mod.capslock)
    {
        return kbdus_capsshft[key_code];
    }
    return 0;
}

// Handle key combinations
void handle_comb(uint8_t scancode)
{
    char ch = get_ascii_char(scancode);

    // Crash the os: CTRL + ALT + DEL
    if (kbd_mod.ctrl && kbd_mod.alt && scancode == keys::DELETE)
    {
        asm volatile ("int $0x3");
        asm volatile ("int $0x4");
    }
    else if (kbd_mod.ctrl && ((ch == 'l') || (ch == 'L')))
    {
        terminal::clear();
        if (reading)
        {
            memset(retstr, '\0', 1024);
            enter = true;
        }
    }
}

// Keyboard buffer
char *buff;
char c[10] = "\0";

// Clear keyboard buffer
void clearbuff()
{
    for (size_t i = 0; i < strlen(buff); i++)
    {
        buff[i] = '\0';
    }
}

// Main keyboard handler
static void Keyboard_Handler(idt::interrupt_registers *)
{
    uint8_t scancode = inb(0x60);

    if (scancode & 0x80)
    {
        switch (scancode)
        {
            case keys::L_SHIFT_UP:
            case keys::R_SHIFT_UP:
                kbd_mod.shift = 0;
                break;
            case keys::CTRL_UP:
                kbd_mod.ctrl = 0;
                break;
            case keys::ALT_UP:
                kbd_mod.alt = 0;
                break;
        }
    }
    else
    {
        switch (scancode)
        {
            case keys::L_SHIFT_DOWN:
            case keys::R_SHIFT_DOWN:
                kbd_mod.shift = 1;
                break;
            case keys::CTRL_DOWN:
                kbd_mod.ctrl = 1;
                break;
            case keys::ALT_DOWN:
                kbd_mod.alt = 1;
                break;
            case keys::CAPSLOCK:
                kbd_mod.capslock = (!kbd_mod.capslock) ? 1 : 0;
                break;
            case keys::NUMLOCK:
                kbd_mod.numlock = (!kbd_mod.numlock) ? 1 : 0;
                break;
            case keys::SCROLLLOCK:
                kbd_mod.scrolllock = (!kbd_mod.scrolllock) ? 1 : 0;
                break;
            case keys::RIGHT:
                strcpy(c, "\033[C");
                terminal::print(c);
                break;
            case keys::LEFT:
                strcpy(c, "\033[D");
                terminal::print(c);
                break;
            default:
                memset(c, 0, strlen(c));
                c[0] = get_ascii_char(scancode);
                if (kbd_mod.alt || kbd_mod.ctrl)
                {
                    handle_comb(scancode);
                }
                else
                {
                    switch (c[0])
                    {
                        case '\n':
                            printf("\n");
                            clearbuff();
                            enter = true;
                            break;
                        case '\b':
                            if (buff[0] != '\0')
                            {
                                buff[strlen(buff) - 1] = '\0';
                                if (reading) retstr[--gi] = 0;
                                printf("\b \b");
                            }
                            break;
                        default:
                            pressed = true;
                            terminal::print(c);
                            strcat(buff, c);
                            break;
                    }
                }
                break;
        }
    }
}

char getchar()
{
    while (!pressed);
    pressed = false;
    return c[0];
}

DEFINE_LOCK(getline_lock)
char *getline()
{
    acquire_lock(&getline_lock);
    reading = true;
    memset(retstr, '\0', 1024);
    while (!enter)
    {
        if (pressed)
        {
            if (gi >= 1024 - 1)
            {
                printf("\nBuffer Overflow");
            }
            retstr[gi] = getchar();
            gi++;
        }
    }
    enter = false;
    reading = false;
    gi = 0;
    release_lock(&getline_lock);
    return retstr;
}

void init()
{
    serial::info("Initialising PS2 keyboard");

    if (initialised)
    {
        serial::info("Keyboard driver has already been initialised!\n");
        return;
    }

    register_interrupt_handler(idt::IRQS::IRQ1, Keyboard_Handler);
    buff[0] = '\0';

    serial::newline();
    initialised = true;
}
}