Vmware ESXi cheat sheet

edit CC in Makefile:
```
CC = /build/toolchain/lin64/gcc-4.4.3-2/bin/x86_64-linux-gcc
```

```
make memdump OPT="-O -DLINUX2"
/build/toolchain/lin64/gcc-4.4.3-2/bin/x86_64-linux-gcc -Wformat -Wunused -O -DLINUX2 -g -I.   -o memdump-esxi-static memdump.o convert_size.o error.o mymalloc.o -static
/build/toolchain/lin64/binutils-2.20.1-1/bin/x86_64-linux-strip memdump-esxi-static
```

