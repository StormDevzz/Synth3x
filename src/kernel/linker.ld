/* Synth3x OS — Linker Script for x86_64 kernel */
OUTPUT_FORMAT(elf32-i386)
ENTRY(_start)

SECTIONS {
    . = 1M;  /* Load at 1MB (multiboot standard) */

    .multiboot : {
        *(.multiboot)
    }

    .text : {
        *(.text)
        *(.text.*)
    }

    .rodata : {
        *(.rodata)
        *(.rodata.*)
    }

    .data : {
        *(.data)
        *(.data.*)
    }

    .bss : {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }

    .note.Xen : {
        KEEP(*(.note.Xen))
    }

    /DISCARD/ : {
        *(.comment)
        *(.note.*)
    }
}
