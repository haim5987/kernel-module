# Virtual File System

This project implements a custom virtual file system (VFS) in the Linux kernel which when mounted contains a read-only file system that contains:<br>

```/hello.txt``` - Contains &quot;Hello world!&quot;<br>
```/calc/fib.num``` - Contains the next number of a fibonacci sequence.

### <b>Compilation</b>

To compile the VFS module, follow these steps:

1. Make sure you have the necessary kernel headers installed on your system.

2. Clone the repository:

```bash
git clone https://github.com/haim5987/kernel-module.git
```
   
 3. Build
 ```bash 
 cd kernel-module
 make
```

### <b>Mounting the VFS</b>
 ```bash 
sudo insmod simple_vfs.ko
sudo mkdir /mnt/<custom-fs-dir>
sudo mount -t vfs none /mnt/<custom-fs-dir>