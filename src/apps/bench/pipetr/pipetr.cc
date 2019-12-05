/*
 * Copyright (C) 2019, Matthias Hille <matthias.hille@tu-dresden.de>, 
 * Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of SemperOS.
 *
 * SemperOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * SemperOS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#include <base/util/Profile.h>

#include <m3/stream/Standard.h>
#include <m3/pipe/DirectPipe.h>
#include <m3/vfs/FileRef.h>

using namespace m3;

enum {
    BUF_SIZE    = 4 * 1024,
    MEM_SIZE    = BUF_SIZE * 128,
};

alignas(DTU_PKG_SIZE) static char buffer[BUF_SIZE];

NOINLINE void replace(char *buffer, long res, char c1, char c2) {
    long i;
    for(i = 0; i < res; ++i) {
        if(buffer[i] == c1)
            buffer[i] = c2;
    }
}

int main(int argc, char **argv) {
    if(argc < 5)
        exitmsg("Usage: " << argv[0] << " <in> <out> <s> <r>");

    if(VFS::mount("/", new M3FS("m3fs")) < 0)
        exitmsg("Mounting root-fs failed");

    cycles_t apptime = 0;
    cycles_t start = Profile::start(0);

    VPE writer("writer");
    DirectPipe pipe(VPE::self(), writer, MEM_SIZE);

    writer.mountspace(*VPE::self().mountspace());
    writer.obtain_mountspace();

    writer.fds()->set(STDIN_FD, VPE::self().fds()->get(STDIN_FD));
    writer.fds()->set(STDOUT_FD, VPE::self().fds()->get(pipe.writer_fd()));
    writer.obtain_fds();

    writer.run([argv] {
        FileRef input(argv[1], FILE_R);
        if(Errors::occurred())
            exitmsg("open of " << argv[1] << " failed");

        size_t res;
        File *out = VPE::self().fds()->get(STDOUT_FD);
        while((res = input->read(buffer, sizeof(buffer))) > 0)
            out->write(buffer, res);
        return 0;
    });

    pipe.close_writer();

    {
        FileRef output(argv[2], FILE_W | FILE_CREATE | FILE_TRUNC);
        if(Errors::occurred())
            exitmsg("open of " << argv[2] << " failed");

        char c1 = argv[3][0];
        char c2 = argv[4][0];

        size_t res;
        File *in = VPE::self().fds()->get(pipe.reader_fd());
        while((res = in->read(buffer, sizeof(buffer))) > 0) {
            cycles_t cstart = Profile::start(0xbbbb);
            replace(buffer, res, c1, c2);
            cycles_t cend = Profile::stop(0xbbbb);
            apptime += cend - cstart;
            output->write(buffer, res);
        }
    }

    pipe.close_reader();
    writer.wait();

    cycles_t end = Profile::stop(0);
    cout << "Total time: " << (end - start) << " cycles\n";
    cout << "App time: " << apptime << " cycles\n";
    return 0;
}
