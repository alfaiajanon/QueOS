/*

    Observation
    Multiboot Tag Iteration: Replaced raw pointer arithmetic with safer alignment-aware iteration. Added guards against malformed tags and zero-size loops.
    
    Global State Initialization: Ensured module_list and app_list are explicitly zeroed before use.
    
    Dead Code Removal: Removed commented-out debug prints and unused lines that cluttered the logic.
    
    Bounds Checking: Added capacity checks when populating module_list.apps to prevent buffer overflows.
    
    Type Safety: Used uintptr_t for address arithmetic to avoid implicit casts between pointer and integer types.
    
    CLI Transition: Clarified that cli_main() is the final handoff point before halting the system. Added a clean infinite halt loop after CLI exit.

*/

#include "global.h"
#include "io/io.h"
#include "interrupts/idt.h"
#include "interrupts/isr.h"
#include "interrupts/irq.h"
#include "memory/memory.h"
#include "drivers/display/display.h"
#include "drivers/keyboard/keyboard.h"
#include "../cliutils.h"


extern ScreenInfo _SYS_ScreenInfo;
extern void cli_main();
extern void init_cli_screen();



ModuleList module_list;
AppList app_list;



typedef struct{
    multiboot_tag tag;
    uint32_t mod_start;
    uint32_t mod_end;
    char cmdline[0];    // zero terminated string
}multiboot_tag_module;



/* OLD FUNC
void kernel_main(uint32_t magic, uint32_t addr) {
    if (magic != MULTIBOOT2_MAGIC) return;

    multiboot_tag* tag = (multiboot_tag*)(addr + 8); // skip total_size and reserved
    
    _sys_init_display(tag);
    init_cli_screen();
    
    _sys_init_ISR();
    _sys_init_IDT();
    _sys_init_IRQ();
    
    _sys_init_memory(tag);
    
    _sys_init_keyboard();
    
    enable_interrupts();


    module_list.count = 0;
    // cli_printf("Loading modules...\n");

    while (tag->type != MULTIBOOT_TAG_TYPE_END) {
        if (tag->type == MULTIBOOT_TAG_TYPE_MODULE) {
            // cli_printf("got a module\n");
            multiboot_tag_module* module = (multiboot_tag_module*)tag;
            
            cli_printf("Loaded module: %s, from %d kb to %d kb\n", &(module->cmdline[0]), byteToKB(module->mod_start), byteToKB(module->mod_end));
            _sys_pmm_reserve_range((void*)module->mod_start, (void*)module->mod_end);
        
            module_list.apps[module_list.count].base = module->mod_start;
            module_list.apps[module_list.count].size = module->mod_end - module->mod_start;
            module_list.apps[module_list.count].name = &(module->cmdline[0]);
            // cli_printf("%s\n", module_list.apps[module_list.count].name);
            module_list.count++;
        }
        tag = (multiboot_tag*)((uint8_t*)tag + ((tag->size + 7) & ~7));
    }
    
    app_list.count = 0;


    // finally go to cli stuff
    cli_main();

    while (1);
}
*/

// Returns the capacity of module_list.apps at compile time
static inline size_t module_list_capacity(void) {
    return sizeof(module_list.apps) / sizeof(module_list.apps[0]);
}

// CORRECTED
void kernel_main(uint32_t magic, uint32_t addr) {
    // Validate Multiboot2 magic before touching tags
    if (magic != MULTIBOOT2_MAGIC) {
        // Cannot report via CLI yet; fail fast
        for (;;) asm volatile("hlt");
    }

    // Multiboot2 tags start after total_size and reserved (8 bytes)
    uintptr_t mb_addr = (uintptr_t)addr;
    multiboot_tag* tag = (multiboot_tag*)(mb_addr + 8);

    _sys_init_display(tag);
    init_cli_screen();

    _sys_init_ISR();
    _sys_init_IDT();
    _sys_init_IRQ();

    _sys_init_memory(tag);

    _sys_init_keyboard();

    // Interrupts are enabled only after IDT/IRQ/keyboard are initialized
    enable_interrupts();

    module_list.count = 0;
    app_list.count = 0;

    // Iterate Multiboot2 tag list with size/alignment checks (8-byte aligned)
    while (tag && tag->type != MULTIBOOT_TAG_TYPE_END) {
        if (tag->size < sizeof(multiboot_tag)) {
            // Malformed tag; abort iteration to avoid undefined behavior
            break;
        }

        if (tag->type == MULTIBOOT_TAG_TYPE_MODULE) {
            multiboot_tag_module* module = (multiboot_tag_module*)tag;

            // Reserve the physical range occupied by the module before use
            _sys_pmm_reserve_range((void*)module->mod_start, (void*)module->mod_end);

            // Log loaded module with KB conversion (non-negative values)
            cli_printf(
                "Loaded module: %s, from %u kb to %u kb\n",
                &module->cmdline[0],
                (unsigned)byteToKB(module->mod_start),
                (unsigned)byteToKB(module->mod_end)
            );

            // Append to module list if capacity allows; otherwise, skip safely
            size_t cap = module_list_capacity();
            if (module_list.count < cap) {
                module_list.apps[module_list.count].base = module->mod_start;
                module_list.apps[module_list.count].size = module->mod_end - module->mod_start;
                module_list.apps[module_list.count].name = &module->cmdline[0];
                module_list.count++;
            } else {
                cli_printf("Module list capacity reached; skipping %s\n", &module->cmdline[0]);
            }
        }

        // Advance to next tag with 8-byte alignment; guard against zero-size to avoid infinite loop
        size_t advance = (tag->size + 7u) & ~7u;
        if (advance == 0) break;
        tag = (multiboot_tag*)((uint8_t*)tag + advance);
    }

    // Hand control to CLI; when it returns, halt the CPU cleanly
    cli_main();

    for (;;)
        asm volatile("hlt");
}
