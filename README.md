# Tugas Besar IF2230 Sistem Operasi

## Table of Contents
- [Tugas Besar IF2230 Sistem Operasi](#tugas-besar-if2230-sistem-operasi)
  - [Table of Contents](#table-of-contents)
  - [Project Description](#project-description)
  - [Team Member](#team-member)
  - [Running The Program](#running-the-program)
  - [Program Structure](#program-structure)

## Project Description

This Repository is meant for the Operating System Course on making a Operating System, specifically, a 32-bit Operating System. 

Milestone 1 focuses on the GDT, Writing text to a framebuffer and booting sequence

Milestone 2 focuses on interrupt, keyboard I/O, and Filesystem, the bonus included in Milestone 2 includes Infinite Directory entry for the filesystem, CMOS Time, and designing/implementing a filesystem other than the Filesystem FAT32 - IF2230 Edition

## Team Member
| NIM      | Nama                       |
| -------- | -------------------------- |
| 13521045 | Fakhri Muhammad Mahendra   |
| 13521089 | Kenneth Ezekiel Suprantoni |
| 13521093 | Akbar Maulana Ridho        |
| 13521101 | Arsa Izdihar Islam         |

## Running The Program

Run the makefile using `make` command, all of the dependencies will automatically be compiled, and a kernel window will pop up using the QEMU Emulator.

for creating a disk image, use `make disk` command, this will create a new disk image and delete the last one.

## Program Structure

```
│  
│   README
│   makefile
│   .gitignore
│
├─── bin
│        
├─── other
│        
└─── src
      │    cmos.c
      │    framebuffer.c
      │    gdt.c
      │    kernel_loader.s
      │    kernel.c
      │    keyboard.c
      │    linker.ld
      │    math.c
      │    menu.lst
      │    portio.c
      │    stdmem.c
      │    string.c
      │    
      ├─── Filesystem
      ├─── Interrupt
      └─── lib-header

```

## EXT2 Filesystem
This project implements EXT2 as Operating System Filesystem. We design the C header structure by ourself. The source of our EXT2 filesystem can be accessed [here](https://www.nongnu.org/ext2-doc/ext2.html).

### Directory Entry
We use linked list directory table that contains directory entry. The directory table is also a linked list so that if the entry is already full in a block, another block will be allocated (infinite directory).

Each directory entry has dynamic length of name. The implementation is almost like this.

<img src="./img/directory-entry.png" />