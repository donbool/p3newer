Study: WAD File System Implementation / FUSE Daemon

Project Overview
This project is a low-level exploration of the WAD file format used in old games like DOOM. The goal is to create a userspace file system library and daemon using the FUSE (Filesystem in Userspace) API to make the WAD file structure tangible in user space.

WAD File Format
WAD stands for "Where's All the Data?" and is a format used to store various game data such as textures, sounds, maps, and other game elements, collectively referred to as lump data. The format is an efficient way to package and organize large amounts of game content into a monolithic binary file.

Descriptors in the WAD file format indicate the location of elements (which could be files or directories) within the lump data.
Map Markers represent directories, while game levels are represented as files within those directories.
Namespaces are used to group content and files together.
Data Structures
The system is built using key data structures to manage and access the WAD file format efficiently:

N-ary Tree: A data structure to represent the relationships and traversals between files and directories in the WAD system. This improves the efficiency of accessing game data.
Map: Provides access to nodes based on paths, helping to locate specific files or directories.
Descriptors: Used to declare the structure of each file and directory in the system.
The N-ary tree, descriptors, and map work together to represent and navigate the WAD file system.

Library Overview
The library supports basic file system operations such as:

writeToFile: Writes content to a file within the WAD system.
createFile: Creates a new file.
createDirectory: Creates a new directory.
FUSE Daemon Operations
Using the FUSE API, this daemon enables file system operations like:

ls: Lists the contents of a directory.
cp: Copies a file.
mkdir: Creates a directory.
touch: Creates an empty file.
mknod: Creates a special file (like a device file).
cat: Displays the content of a file.
write: Writes data to a file.
Testing & Validation
The project includes 34 unit tests that compare the output of various functions to sample text files. These tests validate the file system's functionality, including file and directory creation, writing operations, and the overall integrity of the daemon.

Unit tests focus on library functions.
The working of the daemon itself serves as a comprehensive test of the system.
