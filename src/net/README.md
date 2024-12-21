# Add Networking Support

I have completed the 6th lab of xv6-riscv. I hope to
also add network support for my FDUOS.

If time should allow, I may consider adding a TCP/IP protocol
stack.

# Currently Support

<ul>
    <li> An UDP/IP stack; </li>
    <li> Packet transmission; </li>
    <li> Package reception(DMA & Interrupt); </li>
</ul>

# Tests

## net_test.c

Usage: start the `server.py 23456` and then `make net-test` in the build directory.

# Bugs

~~When receiving a packet, should use DMA/Interrupt instead of polling for 
performances~~.(FIXED) 

# Copyright

Much code in this module is adapted from the starter code of networking lab of 
6.828, [url](https://pdos.csail.mit.edu/6.828/2021/labs/net.html). Changes I made
include: register the interrupt ID at gicv3; translate riscv64 virt's mmio address 
space to aarch64 virt's mmio address space, fix missing function of E1000 driver, and 
configure `qemu-system-aarch64` for networking. 

The copyright notice and this permission notice of the 
code is reproduced in full below.

```
The xv6 software is:

Copyright (c) 2006-2024 Frans Kaashoek, Robert Morris, Russ Cox,
                        Massachusetts Institute of Technology

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```
