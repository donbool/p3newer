#ifndef WAD_H
#define WAD_H

#include <string>
#include <vector>
#include <map>

struct Node { //for n-ary tree structure
    std::string name;
    bool isDirectory;
    std::vector<Node*> children; 
    int offset; 
    int length;
    Node* parent;
};

struct Descriptor { //descriptor vars
    int elementOffset; //4 bytes. where the contents of the file are
    int elementLength; //4 bytes, size of the contents of the lump data in wad file (name of file)
    std::string descriptorName; //8 bytes
};

//contents of file are stored in the lump data. use the element offset to get there

class Wad {
public:
    Wad(const std::string &wadPath);
    ~Wad();

    //functions from project spec
    static Wad* loadWad(const std::string &path);
    std::string getMagic();
    bool isContent(const std::string &path);
    bool isDirectory(const std::string &path);
    int getSize(const std::string &path);
    int getContents(const std::string &path, char *buffer, int length, int offset = 0);
    int getDirectory(const std::string &path, std::vector<std::string> *directory);
    void createDirectory(const std::string &path);
    void createFile(const std::string &path);
    int writeToFile(const std::string &path, const char *buffer, int length, int offset = 0);

    //functions i added
    void initializeDescriptors(std::ifstream &file);
    void processDescriptors();
    void addToPathMap(const std::string& path, Node* node);
    Node* getNodeByPath(const std::string& path);
    void addNamespaceMarkersToDescriptorList(const std::string &dirName, Node* parentNode);
    void addDirectoryToTreeAndMap(const std::string &dirName, Node* parentNode, const std::string &fullPath);
    std::string extractDirectoryName(const std::string& path);
    std::string extractParentPath(const std::string& path);
    void addFileDescriptorToParent(const Descriptor& newFileDescriptor, Node* parentNode);
    void writeDataToLump(const char* buffer, int length, long offset);
    void moveDescriptorListAndWriteNewData(Node* fileNode);
    void updateDescriptorListInFile(std::fstream &wadFile, long descriptorListStart);
    void writeDescriptor(std::fstream& wadFile, const Descriptor& descriptor);
    void writeUpdatedDescriptors(std::fstream &wadFile);
    void updateDescriptorOffsets(const std::string &newDirName, long shiftSize);
    void printTree(Node* node, int level);
    bool isNamespaceDirectory(Node* node);
    void insertDescriptors(Node* parentNode, Descriptor& startDescriptor, Descriptor& endDescriptor);
    void writeUpdatedDescriptorsToFile();
    void printDescriptorList();
    void initializeWad();
    void printDescriptorsFromFile();
    void updateHeader(std::fstream& wadFile);
    Node* getParentNodeByPath(const std::string &path);
    void updateHeader();
    int moveDescriptorsAndLumpData(int length);
    Node* getNodeForDirectoryCreation(const std::string &path);
    std::string extractParentPathForFiles(const std::string& path);
    void insertDescriptorForFiles(Node* parentNode, Descriptor& fileDescriptor);
    void addFileToTreeAndMap(const std::string& dirName, Node* parentNode, const std::string& fullPath);
    void updateFileHeader();
    void shiftDescriptorOffsets(int shiftOffset, std::streampos oldDescriptorOffset);
    void updateDescriptorOffset(std::fstream& wadFile, std::streampos newDescriptorOffset);
    void updateFileHeader(std::fstream& wadFile);
    void updateDescriptorOffsetInHeader(std::fstream& file, int newOffset);
    void updateDescriptorOffset(std::fstream& wadFile);
    void shiftDescriptorOffsets(int shiftOffset);
    void writeDescriptorsToFile();
    void shiftDescriptorOffsets(Descriptor& desc, int shiftOffset);
    void updateFileHeader(std::fstream& wadFile, int length);
    void updateDescriptorOffsetInHeader(std::fstream& wadFile);
    void updateNodeInTree(Node* currentNode, const std::string& path, int newOffset, int newLength);
    void updateDescriptorOffsets(int shiftStartOffset);
    void shiftFileContent(std::fstream& file, int startOffset);
    void insertDescriptorsWritingToWad(Node* parentNode, Descriptor startDescriptor, Descriptor endDescriptor);

    Node* getRoot() const {
        return root;
    }

private:
    // member variables to store WAD file information
    Node* root; //for n-ary tree

    //file header stuff
    std::string magic; //4 bytes
    int numDescriptors; //4 bytes
    int descriptorOffset; //8 bytes
    int descriptorLength; //8 bytes

    std::vector<Descriptor> descriptors; //descriptor list
    std::map<std::string, Node*> pathMap; //stores n-ary tree like in a map
    std::string wadPath; 
};

#endif // WAD_H
