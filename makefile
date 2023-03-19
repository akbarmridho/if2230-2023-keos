# Compiler & linker
ASM           = nasm
LIN           = ld
CC            = gcc

# Directory
SOURCE_FOLDER = src
OUTPUT_FOLDER = bin
ISO_NAME      = os2023

# Flags
WARNING_CFLAG = -Wall -Wextra -Werror
DEBUG_CFLAG   = -ffreestanding -fshort-wchar -g
STRIP_CFLAG   = -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs
CFLAGS        = $(DEBUG_CFLAG) $(WARNING_CFLAG) $(STRIP_CFLAG) -m32 -c -I$(SOURCE_FOLDER)
AFLAGS        = -f elf32 -g -F dwarf
LFLAGS        = -T $(SOURCE_FOLDER)/linker.ld -melf_i386

# Genisoimage Flags
BOOT_FLAG	  = -b boot/grub/grub1 -no-emul-boot -boot-load-size 4
IO_FLAG		  = -A os -input-charset utf8 -quiet -boot-info-table -o ${OUTPUT_FOLDER}/${ISO_NAME}.iso $(OUTPUT_FOLDER)/iso

run: all
	@qemu-system-i386 -s -S -cdrom $(OUTPUT_FOLDER)/$(ISO_NAME).iso
all: build
build: iso
clean:
	rm -rf *.o *.iso $(OUTPUT_FOLDER)/kernel



kernel:

	# INTERRUPT_SOURCES = $(wildcard ${SOURCE_FOLDER}/interrupt/*.c)
	# INTERRUPT_OBJECTS = $(patsubst ${SOURCE_FOLDER}/interrupt/%.c,${OUTPUT_FOLDER}/%.o,$(INTERRUPT_SOURCES))
	
	# $(INTERRUPT_OBJECTS): bin/%.o: interrupt/%.c
	# $(CC) $(CFLAGS) $< -o $@


	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/interrupt/interrupt.c -o ${OUTPUT_FOLDER}/interrupt.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/keyboard.c -o ${OUTPUT_FOLDER}/keyboard.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/interrupt/idt.c -o ${OUTPUT_FOLDER}/idt.o
	@$(ASM) $(AFLAGS) $(SOURCE_FOLDER)/interrupt/intsetup.s -o ${OUTPUT_FOLDER}/intsetup.o
	@$(ASM) $(AFLAGS) $(SOURCE_FOLDER)/kernel_loader.s -o $(OUTPUT_FOLDER)/kernel_loader.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/stdmem.c -o ${OUTPUT_FOLDER}/stdmem.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/gdt.c -o ${OUTPUT_FOLDER}/gdt.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/framebuffer.c -o ${OUTPUT_FOLDER}/framebuffer.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/portio.c -o ${OUTPUT_FOLDER}/portio.o  
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/kernel.c -o ${OUTPUT_FOLDER}/kernel.o 
	@$(LIN) $(LFLAGS) ${OUTPUT_FOLDER}/*.o -o $(OUTPUT_FOLDER)/kernel
	@echo Linking object files and generate elf32...
	@rm -f $(OUTPUT_FOLDER)/*.o

iso: kernel
	@mkdir -p $(OUTPUT_FOLDER)/iso/boot/grub
	@cp $(OUTPUT_FOLDER)/kernel     $(OUTPUT_FOLDER)/iso/boot/
	@cp other/grub1                 $(OUTPUT_FOLDER)/iso/boot/grub/
	@cp $(SOURCE_FOLDER)/menu.lst   $(OUTPUT_FOLDER)/iso/boot/grub/
	@genisoimage -R $(BOOT_FLAG) $(IO_FLAG)
	@rm -r $(OUTPUT_FOLDER)/iso/