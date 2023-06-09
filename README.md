# Tugas Besar IF2230 Sistem Operasi

## System Requirements

- GNU GCC 11 (need some adjustment if it is compiled in GCC 9)

## Table of Contents
- [Tugas Besar IF2230 Sistem Operasi](#tugas-besar-if2230-sistem-operasi)
  - [Table of Contents](#table-of-contents)
  - [Project Description](#project-description)
  - [Team Member](#team-member)
  - [Running The Program](#running-the-program)
  - [Program Structure](#program-structure)
  - [EXT2 Filesystem](#ext2-filesystem)
    - [Directory Entry](#directory-entry)
  - [Snake Game](#snake-game)
  - [Simple Text Editor (nano)](#simple-text-editor-nano)

## Project Description

This Repository is meant for the Operating System Course on making a Operating System, specifically, a 32-bit Operating System.

Milestone 1 focuses on the GDT, Writing text to a framebuffer and booting sequence

Milestone 2 focuses on interrupt, keyboard I/O, and Filesystem, the bonus included in Milestone 2 includes Infinite Directory entry for the filesystem, CMOS Time, and designing/implementing a filesystem other than the Filesystem FAT32 - IF2230 Edition

Milestone 3 focuses on user mode and shell. Bonus included in this milestone include recursive cp and rm, relative pathing, and some creativity, for example simple memory management implementation (malloc, free, and realloc), time command, help command, simple text editor program, and snake minigame.

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

for inserting the shell, use `make insert-shell` command, this will insert a shell into the user space

## Program Structure

```
│
│   README
│   makefile
│   .gitignore
│
├─── bin
│
├─── img
│
├─── other
│
└─── src
      │    cmos.c
      │    external-inserter.c
      │    framebuffer.c
      │    gdt.c
      │    kernel_loader.s
      │    kernel.c
      │    keyboard.c
      │    linker.ld
      │    math.c
      │    menu.lst
      │    paging.c
      │    portio.c
      │    stdmem.c
      │    string.c
      │
      ├─── filesystem
      ├─── interrupt
      ├─── lib-header
      └─── user

```

## EXT2 Filesystem
This project implements EXT2 as Operating System Filesystem. We design the C header structure by ourself. The source of our EXT2 filesystem can be accessed [here](https://www.nongnu.org/ext2-doc/ext2.html).

### Directory Entry
We use linked list directory table that contains directory entry. The directory table is also a linked list so that if the entry is already full in a block, another block will be allocated (infinite directory).

Each directory entry has dynamic length of name. The implementation is almost like this.

<img src="./img/directory-entry.png" />

## Snake Game

You can play snake minigame by using command `snake` in shell.

## Simple Text Editor (nano)

You could create a new file and write its concent with command `nano filename`. It will show a nano-like text editor. To save the changes, simply type Ctrl + C. This is write only and does not support edit existing file.