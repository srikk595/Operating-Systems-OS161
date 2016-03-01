#include <file_write.h>
#include <syscall.h>
#include <types.h>
#include <copyinout.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <lib.h>
#include <vfs.h>
#include <current.h>
#include <proc.h>
#include <uio.h>

int sys_write(int fd, const userptr_t buf, size_t nBytes, int* retval)
{
	
	if(fd < 0 || fd >= OPEN_MAX )
		return EBADF;
	
	struct file_handle* fh = get_file_handle(curthread->t_file_table, fd);
	if(fh == NULL)
		return EBADF;

	if(fh->openflags != O_WRONLY && fh->openflags != O_RDWR) // Again, this needs to be changed based on the appended flags.
		return EBADF;



 // !!!! Should we do copyin's to kernel space, or will the VOP_WRITE take care of the invalid address issue for us.

	lock_acquire(fh->fh_lock); // IS this really necessary??.. turns out it is , offset should be synchronized. imagine if parent and child call this at the same time.
	struct iovec iov;
	struct uio u;
	int result;

	iov.iov_ubase = buf;
	iov.iov_len = nBytes;		   // length of the memory space
	u.uio_iov = &iov;
	u.uio_iovcnt = 1;
	u.uio_resid = nBytes;          // amount to write to the file
	u.uio_offset = fh->offset;
	u.uio_segflg =  UIO_USERSPACE;
	u.uio_rw = UIO_WRITE;
	u.uio_space = proc_getas();    // lifted from loadelf.c, is this the right way to do it?

	result = VOP_READ(fh->file, &u);
	if (result) {
		lock_release(fh->fh_lock);
		return result;
	}

	if (u.uio_resid != 0) {
		kprintf("ELF: short read on segment - file truncated?\n");
	}

	// should update offset in the file handle.use lock. uio_offset will be updated. can use it directly.
	fh->offset = u.uio_offset;

	lock_release(fh->fh_lock);

	*retval = nBytes - u.uio_resid; // number of bytes gets returned to the user
	return 0; 


}
