# Simple 

*Simple* is really simple character driver that acts on memory area. It is 
inspired by *scull* from *ldd3* but much more simpler in design. Purpose of 
this driver is to explore basic principles of character device drivers.   

Memory area is dynamically allocated array of 256 bytes. Pointer *data* from
datastructure *simple* is pointing to it. It is possible to write to it or read 
from it by using *read* and *write* methods from datastructure *file_operations*
(definied in <root_linux_source_tree>/include/linux/fs.h). 

# Build 

To build this module, simply run:
```bash
$ make
```
Load module with:
```bash
$ sudo insmod simple.ko
```
Write to memory area:
```bash
# echo test > /dev/simple
```
Read from memory area:
```bash
# cat /dev/simple
```
