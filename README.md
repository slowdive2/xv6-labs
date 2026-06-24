## Kernel Modules Implemented

### mmap
Implemented memory-mapped files in xv6 through the addition of per-process virtual memory areas (VMAs). mmap is able to reserve address space through VMA semantics and to associate the regions with specific files. 
This inherently required a restructuring of the xv6 page fault handler. In favor of efficiency, physical memory population and page allocation is deferred to access time, as opposed to an immediate population of the VMA. 
Implemented munmap() to remove mappings, flush modified pages back to disk for MAP_SHARED, and release associated resources.
Adherence to exit() semantics is followed: on process exit, VMAs are cleanly uninstalled from the process's address space and memory. 
fork() maintains transparent ownership between parent and child processes, with direct copies being installed into the child process during fork time. MAP_SHARED file edits are deferred until the parent process removes the corresponding VMA mappings (either through exit() or munmap()).

### Trap handling
Implemented stack backtracing and experimented with clock ticks by implementing a periodic alarm handler.

### Copy-on-write (CoW)
Extended xv6's fork() to adhere to CoW semantics, which reduces physical memory operations by magnitudes - deferring physical memory duplications until a write page fault is triggered.

### Network driver
Implemented a network driver for the e1000 NIC. Packet reception and transmission via DMA ring buffers in accordance with the E1000 SDM (https://pdos.csail.mit.edu/6.1810/2025/readings/8254x_GBe_SDM.pdf) is adhered to - specifically with respect to DMA ring buffer mechanisms. 
Appropriate coordination between driver and hardware ownership lifetime is therefore ensured. Received packets are delivered from kernelspace to userspace via the existing network subsystem.

### Memory allocator
Redesigned xv6's memory allocator with the intent of minimizing lock contention under multi-core environments. The global kernel memory freelist is replaced with per-CPU freelists, allowing kalloc()/kfree() operations to execute concurrently in such workloads. Implemented memory borrowing so that, in the case where a CPU's freelist is empty, modest borrowing is implemented in order to maintain the availability of physical memory across cores.

### Read-write spinlock 
Some syscalls, such as sys_pause and sys_uptime, are inherently limited by the current capability of xv6's spinlock implementations. This repository implements a read-write spinlock in order to permit concurrent readers while adhering to writer-preference semantics in order to avoid starvation.

### File system extensions
Extended xv6's filesize (~274 kB) limit by providing a doubly-indirect block to each inode. The result is a ~250x maximum size increase. This inherently required modifications to bmap() and itrunc() in order to maintain correctness while handling multi-level block indirection.
Implemented symbolic links by modifying xv6's inode structure, extending open() to properly handle indirect paths (notably handling cyclic inode references), and adherence to O_NOFOLLOW semantics. 

## Testing
Each module branch contains a comprehensive & automated test for the feature being provided. Stress tests for concurrency, memory, and filesystem correctness are validated against for the corresponding feature.

## Original xv6 Documentation

xv6 is a re-implementation of Dennis Ritchie's and Ken Thompson's Unix
Version 6 (v6).  xv6 loosely follows the structure and style of v6,
but is implemented for a modern RISC-V multiprocessor using ANSI C.

ACKNOWLEDGMENTS

xv6 is inspired by John Lions's Commentary on UNIX 6th Edition (Peer
to Peer Communications; ISBN: 1-57398-013-7; 1st edition (June 14,
2000)).  See also https://pdos.csail.mit.edu/6.1810/, which provides
pointers to on-line resources for v6.

The following people have made contributions: Russ Cox (context switching,
locking), Cliff Frey (MP), Xiao Yu (MP), Nickolai Zeldovich, and Austin
Clements.

We are also grateful for the bug reports and patches contributed by
Abhinavpatel00, Takahiro Aoyagi, Marcelo Arroyo, Hirbod Behnam, Silas
Boyd-Wickizer, Anton Burtsev, carlclone, Ian Chen, clivezeng, Dan
Cross, Cody Cutler, Mike CAT, Tej Chajed, Asami Doi,Wenyang Duan,
echtwerner, eyalz800, Nelson Elhage, Saar Ettinger, Alice Ferrazzi,
Nathaniel Filardo, flespark, Peter Froehlich, Yakir Goaron, Shivam
Handa, Matt Harvey, Bryan Henry, jaichenhengjie, Jim Huang, Matúš
Jókay, John Jolly, Alexander Kapshuk, Anders Kaseorg, kehao95,
Wolfgang Keller, Jungwoo Kim, Jonathan Kimmitt, Eddie Kohler, Vadim
Kolontsov, Austin Liew, l0stman, Pavan Maddamsetti, Imbar Marinescu,
Yandong Mao, Matan Shabtay, Hitoshi Mitake, Carmi Merimovich,
mes900903, Mark Morrissey, mtasm, Joel Nider, Hayato Ohhashi,
OptimisticSide, papparapa, phosphagos, Harry Porter, Greg Price, Zheng
qhuo, Quancheng, RayAndrew, Jude Rich, segfault, Ayan Shafqat, Eldar
Sehayek, Yongming Shen, Fumiya Shigemitsu, snoire, Taojie, Cam Tenny,
tyfkda, Warren Toomey, Stephen Tu, Alissa Tung, Rafael Ubal, unicornx,
Amane Uehara, Pablo Ventura, Luc Videau, Xi Wang, WaheedHafez, Keiichi
Watanabe, Lucas Wolf, Nicolas Wolovick, wxdao, Grant Wu, x653, Andy
Zhang, Jindong Zhang, Icenowy Zheng, ZhUyU1997, and Zou Chang Wei.

ERROR REPORTS

Please send errors and suggestions to Frans Kaashoek and Robert Morris
(kaashoek,rtm@mit.edu).  The main purpose of xv6 is as a teaching
operating system for MIT's 6.1810, so we are more interested in
simplifications and clarifications than new features.

BUILDING AND RUNNING XV6

You will need a RISC-V "newlib" tool chain from
https://github.com/riscv/riscv-gnu-toolchain, and qemu compiled for
riscv64-softmmu.  Once they are installed, and in your shell
search path, you can run "make qemu".
