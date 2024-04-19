#include <fuse.h>
#include <string>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include "../libWad/Wad.h"

static int fuse_getattr(const char *path, struct stat *st) {

    Wad* myWad = static_cast<Wad*>(fuse_get_context()->private_data);
    memset(st, 0, sizeof(struct stat));

    if (myWad->isDirectory(path)) {
        st->st_mode = S_IFDIR | 0777;
        st->st_nlink = 2;
        return 0;
    }

    if (myWad->isContent(path)) {
        st->st_mode = S_IFREG | 0777;
        st->st_nlink = 1;
        st->st_size = myWad->getSize(path);
        return 0;
    }

    return -ENOENT;
}

static int fuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;

    Wad* myWad = static_cast<Wad*>(fuse_get_context()->private_data);

    if (strcmp(path, "/") != 0 && !myWad->isDirectory(path)) {
        return -ENOENT;
    }

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    std::vector<std::string> entries;
    myWad->getDirectory(path, &entries);
    for (const std::string &entry : entries) {
        if (filler(buf, entry.c_str(), NULL, 0) != 0) {
            return 0;
        }
    }
    return 0;
}

static int fuse_open(const char *path, struct fuse_file_info *fi) {
    Wad* myWad = static_cast<Wad*>(fuse_get_context()->private_data);

    // Check if the file exists in the WAD archive
    if (!myWad->isContent(path)) {
        return -ENOENT;
    }

    return 0;
}

static int fuse_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    Wad* myWad = static_cast<Wad*>(fuse_get_context()->private_data);

    int returnVal = myWad->getContents(path, buf, size, offset);

    return returnVal == -1 ?  -ENOENT : returnVal;
}

static int fuse_mknod(const char *path, mode_t mode, dev_t rdev) {
    Wad* myWad = static_cast<Wad*>(fuse_get_context()->private_data);

    if (!S_ISREG(mode)) {
        return -EINVAL;
    }

    if (myWad->isContent(path)) {
        return -EEXIST;
    }

    myWad->createFile(path);
    return 0;
}

static int fuse_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    Wad* myWad = static_cast<Wad*>(fuse_get_context()->private_data);

    if (!myWad->isContent(path)) {
        return -EEXIST;
    }

    int returnVal = myWad->writeToFile(path, buf, size, offset);
    return returnVal;
}

static int fuse_mkdir(const char *path, mode_t mode) {
    Wad* myWad = static_cast<Wad*>(fuse_get_context()->private_data);

    if (myWad->isDirectory(path)) {
        return -EEXIST;
    }

    myWad->createDirectory(path);
    return 0;
}

static struct fuse_operations operations = {
    .getattr = fuse_getattr,
    .mknod = fuse_mknod,
    .mkdir = fuse_mkdir,
    .open = fuse_open,
    .read = fuse_read,
    .write = fuse_write,
    .readdir = fuse_readdir,
};

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cout << "Not enough arguments provided." << std::endl;
        exit(EXIT_SUCCESS);
    }

    std::string wadPath = argv[argc - 2];
    if (wadPath.at(0) != '/') {
        wadPath = std::string(get_current_dir_name()) + "/" + wadPath;
    }

    Wad* myWad = Wad::loadWad(wadPath);
    if (!myWad) {
        return EXIT_FAILURE;
    }

    argv[argc - 2] = argv[argc - 1]; // Adjust argument for mount point
    argc--;

    return fuse_main(argc, argv, &operations, myWad);
}
