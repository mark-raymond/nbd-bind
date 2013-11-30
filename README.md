# nbd-bind

## Description

`nbd-bind` is a convenient wrapper for using QEMU's `qemu-nbd` to access disk images.

`qemu-nbd`, among other things, allows you to bind a disk image to a block device, with a name of the form `/dev/nbd[0-9]+`. However, there are two problems with this: only root can run `qemu-nbd` or access the resulting block devices, and you can't know the name of the block device in advance (`/dev/nbd0` may already be in use, and unavailable for binding a disk image to). `nbd-bind` solves these two problems by:

* Running as suid root, and checking the calling user has read/write permissions on the disk image and target
* Binding to any available nbd device, and symlinking from the target name to the block devices
* Giving the calling user ownership of the nbd device while their disk image is attached

## Examples

Binding a disk image called `disk-image.vdi`, which has two partitions on it, to `block`:

```
$ nbd-bind disk-image.vdi block
$ ls
block  block-p1  block-p2  disk-image.vdi
$ nbd-unbind block
$ ls
disk-image.vdi
```

`nbd-bind` can be used in conjunction with an appropriate entry in fstab to have a Makefile which installs to a disk image without running with sudo.

fstab entry:

```
/home/<username>/<path>/block-p1 /home/<username>/<path>/mnt auto user,noauto 0 0
```

install target in `/home/<username>/<path>/Makefile`:

```
install: all
        nbd-bind disk-image.vdi block
        mount mnt
        <commands to install to mnt>
        umount mnt
        nbd-unbind block
```

## Installation

If your copy of QEMU isn't located in `/usr/bin`, or you want to install `nbd-bind` somewhere other than `/usr/local/bin`, then edit the QEMUNBD and PREFIX variables in the Makefile. Then run:

`make; sudo make install`
