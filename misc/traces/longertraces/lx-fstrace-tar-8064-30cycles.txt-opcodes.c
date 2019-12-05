// This file has been automatically generated by strace2c.
// Do not edit it!

#include "common/op_types.h"

trace_op_t trace_ops[] = {
    /* #1 = 0x1 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 114052 } },
    /* #2 = 0x2 */ { .opcode = OPEN_OP, .args.open = { 3, "test.tar", O_WRONLY|O_CREAT|O_TRUNC|O_LARGEFILE, 0666 } },
    /* #3 = 0x3 */ { .opcode = FSTAT_OP, .args.fstat = { 0, 3 } },
    /* #4 = 0x4 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 5927 } },
    /* #5 = 0x5 */ { .opcode = STAT_OP, .args.stat = { 0, "/tardata" } },
    /* #6 = 0x6 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 77290 } },
    /* #7 = 0x7 */ { .opcode = OPEN_OP, .args.open = { 4, "/etc/passwd", O_RDONLY, 0 } },
    /* #8 = 0x8 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 3787 } },
    /* #9 = 0x9 */ { .opcode = READ_OP, .args.read = { 334, 4, 4096, 1 } },
    /* #10 = 0xa */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 3121 } },
    /* #11 = 0xb */ { .opcode = CLOSE_OP, .args.close = { 0, 4 } },
    /* #12 = 0xc */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 9848 } },
    /* #13 = 0xd */ { .opcode = OPEN_OP, .args.open = { 4, "/etc/group", O_RDONLY, 0 } },
    /* #14 = 0xe */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 2441 } },
    /* #15 = 0xf */ { .opcode = READ_OP, .args.read = { 304, 4, 4096, 1 } },
    /* #16 = 0x10 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 1446 } },
    /* #17 = 0x11 */ { .opcode = CLOSE_OP, .args.close = { 0, 4 } },
    /* #18 = 0x12 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 8378 } },
    /* #19 = 0x13 */ { .opcode = WRITE_OP, .args.write = { 512, 3, 512, 1 } },
    /* #20 = 0x14 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 2167 } },
    /* #21 = 0x15 */ { .opcode = OPEN_OP, .args.open = { 4, "/tardata", O_RDONLY|O_NONBLOCK|O_DIRECTORY|O_CLOEXEC, 0 } },
    /* #22 = 0x16 */ { .opcode = FSTAT_OP, .args.fstat = { 0, 4 } },
    /* #23 = 0x17 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 7045 } },
    /* #24 = 0x18 */ { .opcode = GETDENTS_OP, .args.getdents = { 240, 4, /* d_reclen < offsetof(struct dirent64, d_name) */0 } },
    /* #25 = 0x19 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 16155 } },
    /* #26 = 0x1a */ { .opcode = STAT_OP, .args.stat = { 0, "/tardata/2048.bin" } },
    /* #27 = 0x1b */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 1541 } },
    /* #28 = 0x1c */ { .opcode = OPEN_OP, .args.open = { 5, "/tardata/2048.bin", O_RDONLY|O_LARGEFILE, 0 } },
    /* #29 = 0x1d */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 21630 } },
    /* #30 = 0x1e */ { .opcode = WRITE_OP, .args.write = { 512, 3, 512, 1 } },
    /* #31 = 0x1f */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 2554 } },
    /* #32 = 0x20 */ { .opcode = SENDFILE_OP, .args.sendfile = { 2097152, 3, 5, NULL, 2097152 } },
    /* #33 = 0x21 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 3289 } },
    /* #34 = 0x22 */ { .opcode = CLOSE_OP, .args.close = { 0, 5 } },
    /* #35 = 0x23 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 22257 } },
    /* #36 = 0x24 */ { .opcode = STAT_OP, .args.stat = { 0, "/tardata/512.bin" } },
    /* #37 = 0x25 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 2074 } },
    /* #38 = 0x26 */ { .opcode = OPEN_OP, .args.open = { 5, "/tardata/512.bin", O_RDONLY|O_LARGEFILE, 0 } },
    /* #39 = 0x27 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 23618 } },
    /* #40 = 0x28 */ { .opcode = WRITE_OP, .args.write = { 512, 3, 512, 1 } },
    /* #41 = 0x29 */ { .opcode = SENDFILE_OP, .args.sendfile = { 524288, 3, 5, NULL, 524288 } },
    /* #42 = 0x2a */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 1058 } },
    /* #43 = 0x2b */ { .opcode = CLOSE_OP, .args.close = { 0, 5 } },
    /* #44 = 0x2c */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 22577 } },
    /* #45 = 0x2d */ { .opcode = STAT_OP, .args.stat = { 0, "/tardata/1024.bin" } },
    /* #46 = 0x2e */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 1896 } },
    /* #47 = 0x2f */ { .opcode = OPEN_OP, .args.open = { 5, "/tardata/1024.bin", O_RDONLY|O_LARGEFILE, 0 } },
    /* #48 = 0x30 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 23023 } },
    /* #49 = 0x31 */ { .opcode = WRITE_OP, .args.write = { 512, 3, 512, 1 } },
    /* #50 = 0x32 */ { .opcode = SENDFILE_OP, .args.sendfile = { 1048576, 3, 5, NULL, 1048576 } },
    /* #51 = 0x33 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 1087 } },
    /* #52 = 0x34 */ { .opcode = CLOSE_OP, .args.close = { 0, 5 } },
    /* #53 = 0x35 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 22329 } },
    /* #54 = 0x36 */ { .opcode = STAT_OP, .args.stat = { 0, "/tardata/256.bin" } },
    /* #55 = 0x37 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 1949 } },
    /* #56 = 0x38 */ { .opcode = OPEN_OP, .args.open = { 5, "/tardata/256.bin", O_RDONLY|O_LARGEFILE, 0 } },
    /* #57 = 0x39 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 23241 } },
    /* #58 = 0x3a */ { .opcode = WRITE_OP, .args.write = { 512, 3, 512, 1 } },
    /* #59 = 0x3b */ { .opcode = SENDFILE_OP, .args.sendfile = { 262144, 3, 5, NULL, 262144 } },
    /* #60 = 0x3c */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 1089 } },
    /* #61 = 0x3d */ { .opcode = CLOSE_OP, .args.close = { 0, 5 } },
    /* #62 = 0x3e */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 22361 } },
    /* #63 = 0x3f */ { .opcode = STAT_OP, .args.stat = { 0, "/tardata/128.bin" } },
    /* #64 = 0x40 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 1919 } },
    /* #65 = 0x41 */ { .opcode = OPEN_OP, .args.open = { 5, "/tardata/128.bin", O_RDONLY|O_LARGEFILE, 0 } },
    /* #66 = 0x42 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 23217 } },
    /* #67 = 0x43 */ { .opcode = WRITE_OP, .args.write = { 512, 3, 512, 1 } },
    /* #68 = 0x44 */ { .opcode = SENDFILE_OP, .args.sendfile = { 131072, 3, 5, NULL, 131072 } },
    /* #69 = 0x45 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 1088 } },
    /* #70 = 0x46 */ { .opcode = CLOSE_OP, .args.close = { 0, 5 } },
    /* #71 = 0x47 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 22222 } },
    /* #72 = 0x48 */ { .opcode = STAT_OP, .args.stat = { 0, "/tardata/4096.bin" } },
    /* #73 = 0x49 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 1926 } },
    /* #74 = 0x4a */ { .opcode = OPEN_OP, .args.open = { 5, "/tardata/4096.bin", O_RDONLY|O_LARGEFILE, 0 } },
    /* #75 = 0x4b */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 23247 } },
    /* #76 = 0x4c */ { .opcode = WRITE_OP, .args.write = { 512, 3, 512, 1 } },
    /* #77 = 0x4d */ { .opcode = SENDFILE_OP, .args.sendfile = { 4194304, 3, 5, NULL, 4194304 } },
    /* #78 = 0x4e */ { .opcode = CLOSE_OP, .args.close = { 0, 5 } },
    /* #79 = 0x4f */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 6432 } },
    /* #80 = 0x50 */ { .opcode = GETDENTS_OP, .args.getdents = { 0, 4, 0, 1024 } },
    /* #81 = 0x51 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 3016 } },
    /* #82 = 0x52 */ { .opcode = CLOSE_OP, .args.close = { 0, 4 } },
    /* #83 = 0x53 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 2704 } },
    /* #84 = 0x54 */ { .opcode = WRITE_OP, .args.write = { 1024, 3, 1024, 1 } },
    /* #85 = 0x55 */ { .opcode = CLOSE_OP, .args.close = { 0, 3 } },
    { .opcode = INVALID_OP } 
};