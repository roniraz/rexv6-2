/**
 * @ Author: SmartPolarBear
 * @ Create Time: 2019-07-02 17:17:08
 * @ Modified by: SmartPolarBear
 * @ Modified time: 2019-07-04 22:21:55
 * @ Description:
 */

#include "xv6/types.h"
#include "xv6/defs.h"
#include "xv6/param.h"
#include "xv6/stat.h"
#include "xv6/fs.h"
#include "xv6/mount.h"
#include "xv6/file.h"

struct mountsw mountsw[NDEV];
struct mountsw *mntswend;
struct fstable fstable[NDEV];

int regfs(int fsid, struct fstable *fs)
{
    fstable[fsid] = *fs;
    return 0;
}

struct fstable *
getfs(int fsid)
{
    return fstable + fsid;
}

int mountdev(int dev, char *path, int fs)
{
    if (mntswend - mountsw >= NDEV)
        return -1;
    mntswend->dev = dev;
    mntswend->dp = namei(path);
    mntswend->fsid = fs;
    mntswend++;
    return 0;
}

int mountpart(char *path, uint partition_number)
{
    struct inode *ip;
    begin_op();
    if (partition_number < 0 || partition_number > 3)
    {
        cprintf("kernel: mount: partition number out of bounds\n");
        end_op();
        return -1;
    }
    if ((ip = namei(path)) == 0)
    {
        cprintf("kernel: mount: path not found\n");
        end_op();
        return -1;
    }
    ilock(ip);
    int ret = insert_mapping(ip, partition_number);
    iunlockput(ip);
    end_op();
    return ret;
}

BOOL ispartition(struct inode *ip)
{
    if (ip->major != NDEVHDA)
        return FALSE;

    int part = ip->minor - 3;

    return part >= 0 && part <= 3;
}

int mount(char *src, char *target, int fs)
{
    begin_op();
    struct inode *ip = namei(src);
    if (ip == 0)
    {
        end_op();
        return -1;
    }

    ilock(ip);
    int type = ip->type, minor = ip->minor, dev = ip->dev;
    iunlockput(ip);
    end_op();

    if (type == T_DEV)
    {
        if (ispartition(ip))
        {
            cprintf("ispartition\n");
            mountpart(target, minor - 3);
        }
        else
        {
            cprintf("not ispartition\n");
            mountdev(dev, target, fs);
        }
    }
}

int unmount(int dev)
{
    struct mountsw *mp;
    for (mp = mountsw; mp != mntswend; mp++)
    {
        if (mp->dev == dev)
        {
            *mp = *(--mntswend);
            return 0;
        }
    }
    return -1;
}

//this mounts the root device
void mountinit(void)
{
    mntswend = mountsw;
    struct fstable deffs = {deffsread, deffswrite};
    regfs(XV6FS, &deffs);
    mountdev(ROOTDEV, "/", XV6FS);
}
