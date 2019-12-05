import os, sys

sys.path.append(os.path.realpath('hw/gem5/configs/example'))
from dtu_fs import *

options = getOptions()
root = createRoot(options)

cmd_list = options.cmd.split(",")

num_mem = 1
num_pes = int(os.environ.get('M3_GEM5_PES'))
fsimg = os.environ.get('M3_GEM5_FS')
mem_pe = 0

pes = []

# create the memory PEs (have to come first)
for i in range(0, num_mem):
    pes.append(createMemPE(root=root,
                           options=options,
                           no=i,
                           size='1024MB',
                           content=fsimg if i == 0 else None))

# create the core PEs
for i in range(0, num_pes):
    pe = createCorePE(root=root,
                      options=options,
                      no=i + num_mem,
                      cmdline=cmd_list[i],
                      memPE=mem_pe,
                      l1size='64kB',
                      l2size=None)
    pes.append(pe)

runSimulation(options, pes)
