#include <fuse.h>
#include <string>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include "../libWad/Wad.h"

//REFERENCES:
// https://engineering.facile.it/blog/eng/write-filesystem-fuse/
// https://maastaar.net/fuse/linux/filesystem/c/2019/09/28/writing-less-simple-yet-stupid-filesystem-using-FUSE-in-C/

static int do_getattr(const char *path, struct stat *st) {

    Wad* myWad = static_cast<Wad*>(fuse_get_context()->private_data); 

    memset(st, 0, sizeof(struct stat));

    if (myWad->isDirectory(path)) {
        st->st_mode = S_IFDIR | 0777; 
        st->st_nlink = 2; 
    } else if (myWad->isContent(path)) {
        st->st_mode = S_IFREG | 0777; 
        st->st_nlink = 1; 
        st->st_size = myWad->getSize(path); 
    } else {
        return -ENOENT; // if the path isn't found
    }

    return 0;
}

static int do_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
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

    for (const std::string &entry : entries) { //for each entry
        if (filler(buf, entry.c_str(), NULL, 0) != 0) {
            return 0;
        }
    }

    return 0;
}

static int do_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    Wad* myWad = static_cast<Wad*>(fuse_get_context()->private_data);

    int value = myWad->getContents(path, buf, size, offset);

    if (value == -1) {
        return -ENOENT;
    } 
    else {
        return value;
    }
}

//for making directories
static int do_mkdir(const char *path, mode_t mode) {
    Wad* myWad = static_cast<Wad*>(fuse_get_context()->private_data);

    if (myWad->isDirectory(path)) { //change to isDirectory, ernesto said in discussion
        return -EEXIST;
    }

    myWad->createDirectory(path);

    return 0;
}

static int do_mknod(const char *path, mode_t mode, dev_t rdev) {
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

//for writing to file
static int do_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    Wad* myWad = static_cast<Wad*>(fuse_get_context()->private_data);

    if (!myWad->isContent(path)) {
        return -EEXIST;
    }

    int value = myWad->writeToFile(path, buf, size, offset);

    return value;
}

//taken from:
//https://maastaar.net/fuse/linux/filesystem/c/2019/09/28/writing-less-simple-yet-stupid-filesystem-using-FUSE-in-C/
//structure for fuse operations MUST BE IN ORDER!!!!
static struct fuse_operations operations = {
    .getattr = do_getattr,
    .readdir = do_readdir,
    .read = do_read,
    .mkdir = do_mkdir,
    .mknod = do_mknod,
    .write = do_write,
};

// int main( int argc, char *argv[] )
// {
// 	return fuse_main( argc, argv, &operations, NULL );
// }

//provided from Ernesto discussion
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

    argv[argc - 2] = argv[argc - 1];
    argc--;

    // ((Wad*) fuse_get_context()->private_data)->getContents;
    return fuse_main(argc, argv, &operations, myWad);
}
