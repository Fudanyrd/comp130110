#include <common/defines.h>
#include <kernel/syscall.h>
#include <kernel/proc.h>
#include <kernel/sched.h>
#include <fs/file1206.h>
#include <net/socket.h>

int sys_socket(u32 raddr, u16 lport, u16 rport)
{
    Socket *sock = sock_open(raddr, lport, rport);
    if (sock == NULL) {
        return -1;
    }

    File **ofile = (File **)(thisproc()->ofile.ofile);
    int id = -1;
    for (int i = 0; i < MAXOFILE; i++) {
        if (ofile[i] == NULL) {
            id = i;
            break;
        }
    }

    if (id < 0) {
        sock_close(sock);
        return -1;
    }

    File *fobj = kalloc(sizeof(File));
    fobj->type = FD_SOCK;
    fobj->ref = 1;
    fobj->sock = sock;
    fobj->readable = fobj->writable = true;
    fobj->off = 0;

    // done.
    ofile[id] = fobj;
    return id;
}
