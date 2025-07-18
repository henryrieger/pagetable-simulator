# Multi-Level Page Table Simulator

This project is a C-based simulation of a multi-level page table system, designed to model how virtual memory is translated into physical memory in modern operating systems. It supports dynamic allocation of page table levels, memory mapping, and retrieval of stored data using virtual addresses.

## Features

- Configurable number of page table levels
- Memory allocation for virtual pages using `aligned_alloc`
- Virtual-to-physical address translation
- Support for reading and writing values to mapped virtual addresses

## Usage

### Compile

Make sure you have GCC installed (or an equivalent C compiler):

```bash
gcc -o pagetable main.c pagetable.c
