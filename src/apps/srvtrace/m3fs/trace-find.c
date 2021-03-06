// This file has been automatically generated by strace2c.
// Do not edit it!

#include "common/op_types.h"

trace_op_t trace_ops[] = {
    /* #1 = 0x1 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 134307 } },
    /* #2 = 0x2 */ { .opcode = STAT_OP, .args.stat = { 0, "/default" } },
    /* #3 = 0x3 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 14102 } },
    /* #4 = 0x4 */ { .opcode = OPEN_OP, .args.open = { 3, "/default", O_RDONLY|O_NONBLOCK|O_DIRECTORY|O_CLOEXEC, 0 } },
    /* #5 = 0x5 */ { .opcode = FSTAT_OP, .args.fstat = { 0, 3 } },
    /* #6 = 0x6 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 7615 } },
    /* #7 = 0x7 */ { .opcode = GETDENTS_OP, .args.getdents = { 368, 3, 12, 1024 } },
    /* #8 = 0x8 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 71055 } },
    /* #9 = 0x9 */ { .opcode = STAT_OP, .args.stat = { 0, "/default/largedir" } },
    /* #10 = 0xa */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 2428 } },
    /* #11 = 0xb */ { .opcode = OPEN_OP, .args.open = { 4, "/default/largedir", O_RDONLY|O_NONBLOCK|O_DIRECTORY|O_CLOEXEC, 0 } },
    /* #12 = 0xc */ { .opcode = FSTAT_OP, .args.fstat = { 0, 4 } },
    /* #13 = 0xd */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 5400 } },
    /* #14 = 0xe */ { .opcode = GETDENTS_OP, .args.getdents = { 880, 4, 28, 1024 } },
    /* #15 = 0xf */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 30169 } },
    /* #16 = 0x10 */ { .opcode = STAT_OP, .args.stat = { 0, "/default/largedir/4.txt" } },
    /* #17 = 0x11 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 15915 } },
    /* #18 = 0x12 */ { .opcode = STAT_OP, .args.stat = { 0, "/default/largedir/16.txt" } },
    /* #19 = 0x13 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 18971 } },
    /* #20 = 0x14 */ { .opcode = STAT_OP, .args.stat = { 0, "/default/largedir/9.txt" } },
    /* #21 = 0x15 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 13570 } },
    /* #22 = 0x16 */ { .opcode = STAT_OP, .args.stat = { 0, "/default/largedir/20.txt" } },
    /* #23 = 0x17 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 13405 } },
    /* #24 = 0x18 */ { .opcode = STAT_OP, .args.stat = { 0, "/default/largedir/1.txt" } },
    /* #25 = 0x19 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 13403 } },
    /* #26 = 0x1a */ { .opcode = STAT_OP, .args.stat = { 0, "/default/largedir/5.txt" } },
    /* #27 = 0x1b */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 13414 } },
    /* #28 = 0x1c */ { .opcode = STAT_OP, .args.stat = { 0, "/default/largedir/14.txt" } },
    /* #29 = 0x1d */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 17326 } },
    /* #30 = 0x1e */ { .opcode = STAT_OP, .args.stat = { 0, "/default/largedir/6.txt" } },
    /* #31 = 0x1f */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 13537 } },
    /* #32 = 0x20 */ { .opcode = STAT_OP, .args.stat = { 0, "/default/largedir/10.txt" } },
    /* #33 = 0x21 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 13408 } },
    /* #34 = 0x22 */ { .opcode = STAT_OP, .args.stat = { 0, "/default/largedir/7.txt" } },
    /* #35 = 0x23 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 13294 } },
    /* #36 = 0x24 */ { .opcode = STAT_OP, .args.stat = { 0, "/default/largedir/21.txt" } },
    /* #37 = 0x25 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 13296 } },
    /* #38 = 0x26 */ { .opcode = STAT_OP, .args.stat = { 0, "/default/largedir/11.txt" } },
    /* #39 = 0x27 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 13296 } },
    /* #40 = 0x28 */ { .opcode = STAT_OP, .args.stat = { 0, "/default/largedir/13.txt" } },
    /* #41 = 0x29 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 17350 } },
    /* #42 = 0x2a */ { .opcode = STAT_OP, .args.stat = { 0, "/default/largedir/12.txt" } },
    /* #43 = 0x2b */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 13408 } },
    /* #44 = 0x2c */ { .opcode = STAT_OP, .args.stat = { 0, "/default/largedir/3.txt" } },
    /* #45 = 0x2d */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 13356 } },
    /* #46 = 0x2e */ { .opcode = STAT_OP, .args.stat = { 0, "/default/largedir/15.txt" } },
    /* #47 = 0x2f */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 13476 } },
    /* #48 = 0x30 */ { .opcode = STAT_OP, .args.stat = { 0, "/default/largedir/25.txt" } },
    /* #49 = 0x31 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 13386 } },
    /* #50 = 0x32 */ { .opcode = STAT_OP, .args.stat = { 0, "/default/largedir/17.txt" } },
    /* #51 = 0x33 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 13296 } },
    /* #52 = 0x34 */ { .opcode = STAT_OP, .args.stat = { 0, "/default/largedir/18.txt" } },
    /* #53 = 0x35 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 17288 } },
    /* #54 = 0x36 */ { .opcode = STAT_OP, .args.stat = { 0, "/default/largedir/22.txt" } },
    /* #55 = 0x37 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 13439 } },
    /* #56 = 0x38 */ { .opcode = STAT_OP, .args.stat = { 0, "/default/largedir/2.txt" } },
    /* #57 = 0x39 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 13313 } },
    /* #58 = 0x3a */ { .opcode = STAT_OP, .args.stat = { 0, "/default/largedir/8.txt" } },
    /* #59 = 0x3b */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 13294 } },
    /* #60 = 0x3c */ { .opcode = STAT_OP, .args.stat = { 0, "/default/largedir/24.txt" } },
    /* #61 = 0x3d */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 13386 } },
    /* #62 = 0x3e */ { .opcode = STAT_OP, .args.stat = { 0, "/default/largedir/19.txt" } },
    /* #63 = 0x3f */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 13375 } },
    /* #64 = 0x40 */ { .opcode = STAT_OP, .args.stat = { 0, "/default/largedir/0.txt" } },
    /* #65 = 0x41 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 17338 } },
    /* #66 = 0x42 */ { .opcode = STAT_OP, .args.stat = { 0, "/default/largedir/23.txt" } },
    /* #67 = 0x43 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 2270 } },
    /* #68 = 0x44 */ { .opcode = GETDENTS_OP, .args.getdents = { 0, 4, 0, 1024 } },
    /* #69 = 0x45 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 3108 } },
    /* #70 = 0x46 */ { .opcode = CLOSE_OP, .args.close = { 0, 4 } },
    /* #71 = 0x47 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 14935 } },
    /* #72 = 0x48 */ { .opcode = STAT_OP, .args.stat = { 0, "/default/zeros.bin" } },
    /* #73 = 0x49 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 15285 } },
    /* #74 = 0x4a */ { .opcode = STAT_OP, .args.stat = { 0, "/default/test.txt" } },
    /* #75 = 0x4b */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 15101 } },
    /* #76 = 0x4c */ { .opcode = STAT_OP, .args.stat = { 0, "/default/movies" } },
    /* #77 = 0x4d */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 2062 } },
    /* #78 = 0x4e */ { .opcode = OPEN_OP, .args.open = { 4, "/default/movies", O_RDONLY|O_NONBLOCK|O_DIRECTORY|O_CLOEXEC, 0 } },
    /* #79 = 0x4f */ { .opcode = FSTAT_OP, .args.fstat = { 0, 4 } },
    /* #80 = 0x50 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 3951 } },
    /* #81 = 0x51 */ { .opcode = GETDENTS_OP, .args.getdents = { 80, 4, 3, 1024 } },
    /* #82 = 0x52 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 17794 } },
    /* #83 = 0x53 */ { .opcode = STAT_OP, .args.stat = { 0, "/default/movies/starwars.txt" } },
    /* #84 = 0x54 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 2441 } },
    /* #85 = 0x55 */ { .opcode = GETDENTS_OP, .args.getdents = { 0, 4, 0, 1024 } },
    /* #86 = 0x56 */ { .opcode = CLOSE_OP, .args.close = { 0, 4 } },
    /* #87 = 0x57 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 14350 } },
    /* #88 = 0x58 */ { .opcode = STAT_OP, .args.stat = { 0, "/default/BitField.h" } },
    /* #89 = 0x59 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 15275 } },
    /* #90 = 0x5a */ { .opcode = STAT_OP, .args.stat = { 0, "/default/SConscript" } },
    /* #91 = 0x5b */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 15066 } },
    /* #92 = 0x5c */ { .opcode = STAT_OP, .args.stat = { 0, "/default/testfile.txt" } },
    /* #93 = 0x5d */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 15164 } },
    /* #94 = 0x5e */ { .opcode = STAT_OP, .args.stat = { 0, "/default/pat.bin" } },
    /* #95 = 0x5f */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 19772 } },
    /* #96 = 0x60 */ { .opcode = STAT_OP, .args.stat = { 0, "/default/subdir" } },
    /* #97 = 0x61 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 1972 } },
    /* #98 = 0x62 */ { .opcode = OPEN_OP, .args.open = { 4, "/default/subdir", O_RDONLY|O_NONBLOCK|O_DIRECTORY|O_CLOEXEC, 0 } },
    /* #99 = 0x63 */ { .opcode = FSTAT_OP, .args.fstat = { 0, 4 } },
    /* #100 = 0x64 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 3916 } },
    /* #101 = 0x65 */ { .opcode = GETDENTS_OP, .args.getdents = { 80, 4, 3, 1024 } },
    /* #102 = 0x66 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 12427 } },
    /* #103 = 0x67 */ { .opcode = STAT_OP, .args.stat = { 0, "/default/subdir/subsubdir" } },
    /* #104 = 0x68 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 2267 } },
    /* #105 = 0x69 */ { .opcode = OPEN_OP, .args.open = { 5, "/default/subdir/subsubdir", O_RDONLY|O_NONBLOCK|O_DIRECTORY|O_CLOEXEC, 0 } },
    /* #106 = 0x6a */ { .opcode = FSTAT_OP, .args.fstat = { 0, 5 } },
    /* #107 = 0x6b */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 4595 } },
    /* #108 = 0x6c */ { .opcode = GETDENTS_OP, .args.getdents = { 80, 5, 3, 1024 } },
    /* #109 = 0x6d */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 27177 } },
    /* #110 = 0x6e */ { .opcode = STAT_OP, .args.stat = { 0, "/default/subdir/subsubdir/testfile.txt" } },
    /* #111 = 0x6f */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 2804 } },
    /* #112 = 0x70 */ { .opcode = GETDENTS_OP, .args.getdents = { 0, 5, 0, 1024 } },
    /* #113 = 0x71 */ { .opcode = CLOSE_OP, .args.close = { 0, 5 } },
    /* #114 = 0x72 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 1376 } },
    /* #115 = 0x73 */ { .opcode = GETDENTS_OP, .args.getdents = { 0, 4, 0, 1024 } },
    /* #116 = 0x74 */ { .opcode = CLOSE_OP, .args.close = { 0, 4 } },
    /* #117 = 0x75 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 21135 } },
    /* #118 = 0x76 */ { .opcode = STAT_OP, .args.stat = { 0, "/default/mat.txt" } },
    /* #119 = 0x77 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 4004 } },
    /* #120 = 0x78 */ { .opcode = GETDENTS_OP, .args.getdents = { 0, 3, 0, 1024 } },
    /* #121 = 0x79 */ { .opcode = WAITUNTIL_OP, .args.waituntil = { 0, 1015 } },
    /* #122 = 0x7a */ { .opcode = CLOSE_OP, .args.close = { 0, 3 } },
    { .opcode = INVALID_OP } 
};
