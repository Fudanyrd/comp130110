# Add Networking Support

I have completed the 6th lab of xv6-riscv. I hope to
also add network support for my FDUOS.

If time should allow, I may consider adding a TCP/IP protocol
stack.

# Currently Support

<ul>
    <li> An UDP/IP stack; </li>
    <li> Packet transmission; </li>
    <li> Package receiving(polling, not DMA); </li>
</ul>

# Tests

## net_test.c

Usage: start the `server.py 23456` and then `make net-test` in the build directory.

# Bugs

When receiving a packet, should use DMA/Interrupt instead of polling for 
performances. 

