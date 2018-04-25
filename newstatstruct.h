// this is not the entire .h file, but just the stat() struct you need

struct stat {
    short type;     // Type of file
    int dev;        // Device number
    uint ino;       // Inode number on device
    short nlink;    // Number of links to file
    uint size;      // Size of file in bytes
    uchar checksum; //checksum of file
};

