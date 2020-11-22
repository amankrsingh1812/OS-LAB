# dd if=/dev/zero of=./btrfs_compress.img bs=1M count=256
# dd if=/dev/zero of=./btrfs_nocompress.img bs=1M count=256

# mkfs.btrfs ./btrfs_compress.img
# mkfs.btrfs ./btrfs_nocompress.img

sudo mkdir /mnt/btrfs_compress
sudo mkdir /mnt/btrfs_nocompress

sudo mount -o compress=zlib ./btrfs_compress.img /mnt/btrfs_compress
sudo mount -o compress=no ./btrfs_nocompress.img /mnt/btrfs_nocompress

sudo chown ${USER}:${USER} /mnt/btrfs_compress
sudo chown ${USER}:${USER} /mnt/btrfs_nocompress
