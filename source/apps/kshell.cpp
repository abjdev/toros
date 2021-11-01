#include <drivers/devices/ps2/keyboard/keyboard.hpp>
#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <system/mm/pfalloc/pfalloc.hpp>
#include <drivers/fs/ustar/ustar.hpp>
#include <system/sched/rtc/rtc.hpp>
#include <system/sched/pit/pit.hpp>
#include <system/mm/heap/heap.hpp>
#include <system/pci/pci.hpp>
#include <lib/string.hpp>
#include <lib/memory.hpp>

using namespace kernel::drivers::display;
using namespace kernel::drivers::fs;
using namespace kernel::drivers;
using namespace kernel::system::sched;
using namespace kernel::system::mm;
using namespace kernel::system;

vfs::fs_node_t *current_path;

namespace kernel::apps::kshell {

void shell_parse(char *cmd, char *arg)
{
    switch (hash(cmd))
    {
        case hash("help"):
            printf("Supported commands:\n");
            printf("- help\t-- This\n");
            printf("- clear\t-- Clear terminal\n");
            printf("- ls\t-- List files\n");
            printf("- free\t-- Get memory info in bytes\n");
            printf("-  -h\t-- Get memory info in MB\n");
            printf("- time\t-- Get current RTC time\n");
            printf("- timef\t-- Get current RTC time (Forever loop)\n");
            printf("- tick\t-- Get current PIT tick\n");
            printf("- pci\t-- List PCI devices\n");
            printf("- crash\t-- Crash whole system\n");
            break;
        case hash("clear"):
            terminal::clear();
            break;
        case hash("ls"):
            ustar::list();
            break;
        case hash("cat"):
            ustar::cat(arg);
            break;
        case hash("cd"):
        {
            if (!strncmp(arg, "../", 3) || !strncmp(arg, "..", 2))
            {
                current_path = current_path->parent;
                break;
            }
            if (!strncmp(arg, "./", 2) || !strncmp(arg, ".", 1)) break;
            vfs::fs_node_t *node = vfs::open(current_path, arg);
            if (!node)
            {
                printf("\033[31mNo such directory!%s\n", terminal::colour);
                return;
            }
            current_path = node;
            break;
        }
        case hash("free"):
        {
            double usable = getmemsize();
            double free = pfalloc::getFreeRam();
            if (!strcmp(arg, "-h"))
            {
                usable = usable / 1024 / 1024;
                free = free / 1024 / 1024;
                printf("Usable memory: %.2f MB\nFree memory: %.2f MB\nUsed memory: %.2f MB\n", usable, free, usable - free);
            }
            else printf("Usable memory: %.0f Bytes\nFree memory: %.0f Bytes\nUsed memory: %.0f Bytes\n", usable, free, usable - free);
            break;
        }
        case hash("time"):
            printf("%s\n", rtc::getTime());
            break;
        case hash("tick"):
            printf("%ld\n", pit::get_tick());
            break;
        case hash("timef"):
            while (true)
            {
                printf("%s", rtc::getTime());
                pit::sleep(1);
                printf("\r\033[2K");
            }
            break;
        case hash("pci"):
            for (uint64_t i = 0; i < pci::pcidevcount; i++)
            {
                printf("%s / %s / %s / %s / %s\n",
                    pci::pcidevices[i]->vendorstr,
                    pci::pcidevices[i]->devicestr,
                    pci::pcidevices[i]->ClassStr,
                    pci::pcidevices[i]->subclassStr,
                    pci::pcidevices[i]->progifstr);
            }
            break;
        case hash("crash"):
            asm volatile ("int $0x3");
            asm volatile ("int $0x4");
            break;
        case hash(""):
            break;
        default:
            printf("\033[31mCommand not found!\033[0m\n");
            break;
    }
}

void run()
{
    if (!current_path) current_path = vfs::getchild(NULL, "/");
    printf("\033[32mroot@kernel:\033[95m~%s%s%s# ", (current_path->name[0] != '/') ? "/" : "", current_path->name, terminal::colour);
    char *command = ps2::kbd::getline();
    char cmd[10] = "\0";

    for (size_t i = 0; i < strlen(command); i++)
    {
        if (command[i] != ' ' && command[i] != '\0')
        {
            char c[2] = "\0";
            c[0] = command[i];
            strcat(cmd, c);
        }
        else break;
    }
    char *arg = strrm(command, cmd);
    arg = strrm(arg, " ");

    shell_parse(cmd, arg);
}
}