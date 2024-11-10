#pragma once
#include <common/defines.h>
#include <fdutil/stdint.h>

/** Master Boot Record Layout */
struct mbr {
    /** Bootstrap code area and disk information */
    char disk_info[446];
    /** Partition Entries */
    char partition_entr_1[16];
    char partition_entr_2[16];
    char partition_entr_3[16];
    char partition_entr_4[16];
    uint8_t magic_1; /* Fixed Value 0x55 */
    uint8_t magic_2; /* Fixed Value 0xaa */
};

NO_RETURN void idle_entry();

/** Initialize the read-only mbr struct(Optional). */
extern void init_mbr();

/** Returns an immutable pointer to the mbr on this machine. 
 * NULL if uninitialized(use init_mbr).
 */
extern struct mbr *this_mbr();
