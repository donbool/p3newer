#include "Wad.h"
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <cstring>
#include <algorithm>
#include <regex>

/* **TO DO:
* 
build pigstep[]
*/

//constructor -> file read initialization and storing and stuff
Wad::Wad(const std::string &wadPath) {
    this->wadPath = wadPath;
}

void deleteNodes(Node* node) {
    for (auto child : node->children) {
        deleteNodes(child); 
    }
    delete node;
}

Wad::~Wad() {
    deleteNodes(root); // Start the recursive deletion from the root
    pathMap.clear();
}

//rules are different for directories
//makes the descriptor list
void Wad::initializeDescriptors(std::ifstream &file) {
    // Seek to the beginning of the descriptor list using descriptorOffset
    file.seekg(descriptorOffset, std::ios::beg);

    // Read each descriptor and add it to the vector
    for (int i = 0; i < numDescriptors; ++i) {
        Descriptor desc;
        file.read(reinterpret_cast<char*>(&desc.elementOffset), sizeof(desc.elementOffset));
        file.read(reinterpret_cast<char*>(&desc.elementLength), sizeof(desc.elementLength));
        
        char nameBuffer[9] = {}; // Make room for 8 characters + null terminator
        file.read(nameBuffer, 8); // Read up to 8 characters for the name
        nameBuffer[8] = '\0'; // Explicitly null terminate the string

        // Trim the read name to remove any trailing null characters
        desc.descriptorName = std::string(nameBuffer).c_str(); 

        std::cout << "Descriptor read from file: Name = " << desc.descriptorName << ", Offset = " << desc.elementOffset << ", Length = " << desc.elementLength << std::endl;

        descriptors.push_back(desc);
    }
}

bool isMapMarker(const std::string& name) {
    return name.size() == 4 && name[0] == 'E' && isdigit(name[1]) && name[2] == 'M' && isdigit(name[3]);
}

bool isNamespaceStartMarker(const std::string& name) {
    return name.size() >= 7 && name.size() <= 8 && name.substr(name.size() - 6) == "_START";
}

bool isNamespaceEndMarker(const std::string& name) {
    return name.size() >= 5 && name.size() <= 6 && name.substr(name.size() - 4) == "_END";
}

std::string getNamespaceName(const std::string& name) {
    size_t suffixPos = name.find('_');
    return (suffixPos != std::string::npos) ? name.substr(0, suffixPos) : "";
}

void Wad::addToPathMap(const std::string& path, Node* node) {
    pathMap[path] = node;

    // Debugging print
    // std::cout << "Added to pathMap: " << path 
    //           << ", Name: " << node->name 
    //           << ", Is Directory: " << (node->isDirectory ? "True" : "False") << std::endl;
}

Node* Wad::getNodeByPath(const std::string &path) {
    std::string modifiedPath = path;

    // Check and remove null character at the end if present
    if (!modifiedPath.empty() && modifiedPath.back() == '\0') {
        modifiedPath.pop_back();
    }

    // Try finding the exact match first
    auto it = pathMap.find(modifiedPath);
    if (it != pathMap.end()) {
        std::cout << "yes path found: " << modifiedPath  << std::endl;
        return it->second;  // Found the node
    }

    // // Remove leading and trailing slashes if present
    if (!modifiedPath.empty() && modifiedPath.back() == '/') {
        modifiedPath.pop_back(); // Remove trailing slash
    }
    
    // if (!modifiedPath.empty() && modifiedPath.front() == '/') {
    //     modifiedPath.erase(0, 1); // Remove leading slash
    // }

    // Try finding the node with the modified path
    it = pathMap.find(modifiedPath);
    if (it != pathMap.end()) {
        std::cout << "yes path found: " << modifiedPath  << std::endl;
        return it->second;  // Found the node
    }

    // If still not found, try with the modified path plus a trailing slash
    if (!modifiedPath.empty() && modifiedPath.back() != '/') {
        std::string directoryPath = modifiedPath + '/';
        it = pathMap.find(directoryPath);
        if (it != pathMap.end()) {
            std::cout << "yes path found: " << directoryPath  << std::endl;
            return it->second;  // Found the node as a directory
        }
    }

    std::cout << "path not found bro: " << modifiedPath  << std::endl;
    return nullptr;  // Path not found
}

void Wad::printTree(Node* node, int level = 0) {
    if (!node) return;  // Base case: If the node is null, return.

    // Print indentations based on the level in the tree
    for (int i = 0; i < level; ++i) {
        std::cout << "    ";
    }

     // Print the node name, type, and whether it's a directory
    std::cout << (node->isDirectory ? "DIR: " : "FILE: ") << node->name << " (isDirectory: " << (node->isDirectory ? "True" : "False") << ")" << "offset: " << node->offset << " length: " << node->length << std::endl;

    // Recursively print all children of this node
    for (auto& child : node->children) {
        printTree(child, level + 1);
    }
}

//i store all the descriptors into a vector and then build an n-ary tree out of it
void Wad::processDescriptors() {
    root = new Node{"", true, {}, 0, 0}; // Initialize root
    Node* currentDirectory = root;
    currentDirectory->isDirectory = true; //IDK IF THIS IS CORRECT
    std::string currentPath = "/";

    addToPathMap(currentPath, root);

    for (size_t i = 0; i < descriptors.size(); ++i) {
        const auto& descriptor = descriptors[i];
        bool yur = isMapMarker(descriptor.descriptorName);

        //std::cout << "Processing descriptor: " << descriptor.descriptorName << std::endl;
        //std::cout << "isMapMarker: " << (yur ? "true" : "false") << " for " << descriptor.descriptorName << std::endl;

        if (isMapMarker(descriptor.descriptorName)) {
            Node* mapDir = new Node{descriptor.descriptorName, true, {}, descriptor.elementOffset, descriptor.elementLength, currentDirectory};
            mapDir->isDirectory = true;
            currentDirectory->children.push_back(mapDir);
            // Compute the full path for the map directory and add it to the path map
            // Print the map path being added
            //std::cout << "current path before adding map path to pathMap: " << currentPath << std::endl;
            std::string mapPath = currentPath + descriptor.descriptorName + "/";
            addToPathMap(mapPath, mapDir);
            //FOR TESTING
                // for (const auto& pair : pathMap) {
                //     std::cout << "Path: " << pair.first << ", Node Name: " << pair.second->isDirectory << std::endl;
                // }
            // Print the map path being added
            //std::cout << "Adding map path to pathMap: " << mapPath << std::endl;

            // Process the next 10 descriptors as files in this map directory
            for (int j = 0; j < 10 && (i + 1 + j) < descriptors.size(); ++j) {
                const auto& fileDescriptor = descriptors[i + 1 + j];
                Node* fileNode = new Node{fileDescriptor.descriptorName, false, {}, fileDescriptor.elementOffset, fileDescriptor.elementLength, mapDir};
                fileNode->isDirectory = false;
                fileNode->length = fileDescriptor.elementLength;
                mapDir->children.push_back(fileNode);
                // Compute the full path for the file and add it to the path map
                std::string filePath = mapPath + fileDescriptor.descriptorName;
                //std::cout << "Adding file/directory path to pathMap: " << filePath << std::endl;
                addToPathMap(filePath, fileNode);
                
                    // In the loadWad or processDescriptors function
    // std::cout << "Loading descriptor: " << descriptor.descriptorName << ", Offset: " << descriptor.elementOffset
    //       << ", Length: " << descriptor.elementLength << std::endl;


            }
            i += 10; // Skip the next 10 descriptors as they are now processed
        } else if (isNamespaceStartMarker(descriptor.descriptorName)) {
            std::string namespaceName = getNamespaceName(descriptor.descriptorName);
            Node* namespaceDir = new Node{namespaceName, true, {}, 0, 0, currentDirectory};
            namespaceDir->isDirectory = true;
            currentDirectory->children.push_back(namespaceDir);
            currentPath += namespaceName + "/"; // Update the current path
            
                // In the loadWad or processDescriptors function
    // std::cout << "Loading descriptor: " << descriptor.descriptorName << ", Offset: " << descriptor.elementOffset
    //       << ", Length: " << descriptor.elementLength << std::endl;


            addToPathMap(currentPath, namespaceDir); // Add namespace directory to map
            //std::cout << "Adding namespace start path to pathMap: " << currentPath << std::endl;
            currentDirectory = namespaceDir; // Enter the namespace directory
        } else if (isNamespaceEndMarker(descriptor.descriptorName)) {
            if (currentDirectory != root) { // Make sure not to go above root
                currentPath.erase(currentPath.rfind(currentDirectory->name), currentDirectory->name.length() + 1); // Remove the current directory name from the path
                currentDirectory = currentDirectory->parent; // Move back to the parent directory
                //std::cout << "Namespace end, updated currentPath: " << currentPath << std::endl;
            }
        } else {
            // Regular files or directories that are not under any special marker
            Node* fileNode = new Node{descriptor.descriptorName, false, {}, descriptor.elementOffset, descriptor.elementLength, currentDirectory};
            fileNode->isDirectory = false;
            std::cout << "FILEDESCRIPTOR ELEMENT LENGTH: " << descriptor.elementLength << std::endl;
            fileNode->length = descriptor.elementLength;
            currentDirectory->children.push_back(fileNode);
            //std::cout << "Adding file/directory path to pathMap: " << currentPath + descriptor.descriptorName << std::endl;
            
                // In the loadWad or processDescriptors function
    // std::cout << "Loading descriptor: " << descriptor.descriptorName << ", Offset: " << descriptor.elementOffset
    //       << ", Length: " << descriptor.elementLength << std::endl;

            
            addToPathMap(currentPath + descriptor.descriptorName, fileNode); // Add file or directory to map
        }
    }

    //printTree(root);
}

void Wad::initializeWad() {
    std::ifstream file(wadPath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open WAD file.");
    }

    // Read the magic number
    char magicBuffer[5] = {}; // Plus one for null-terminator
    file.read(magicBuffer, 4);
    magic = std::string(magicBuffer);

    std::regex magicRegex(".*WAD$");

    if (!std::regex_match(magic, magicRegex)) {
        throw std::runtime_error("Invalid WAD file magic.");
    }

    // Read the number of descriptors and descriptor offset directly,
    // since the system uses little-endian format as well
    file.read(reinterpret_cast<char*>(&numDescriptors), sizeof(numDescriptors));
    file.read(reinterpret_cast<char*>(&descriptorOffset), sizeof(descriptorOffset));

    // Initialize and process descriptors
    initializeDescriptors(file);
    file.close();

    processDescriptors(); // Process descriptors to build the tree

    // Print the tree after reading back from the file
    std::cout << "Tree after reading back from the file:" << std::endl;
    printTree(root);
}

Wad* Wad::loadWad(const std::string &path) {
    Wad* wad = new Wad(path);
    try {
        wad->initializeWad();
    } catch (const std::exception& e) {
        std::cerr << "Error loading WAD file: " << e.what() << std::endl;
        delete wad;
        throw;
    }
    return wad;
}
// returns the WAD file magic string
std::string Wad::getMagic() {
    return magic;
}

// Method to check if the path is content
bool Wad::isContent(const std::string &path) {
    auto node = getNodeByPath(path);
    return node && !node->isDirectory; // Return true if node exists and is not a directory
}

// Method to check if the path is a directory
bool Wad::isDirectory(const std::string &path) {
    if (path == "/") {
        // If the path is the root directory, return true
        return true;
    }

    auto node = getNodeByPath(path);
    return node && node->isDirectory;
}

// Method to get the size of a file
int Wad::getSize(const std::string &path) {
    if (isContent(path)) {  // Check if the path represents content
        Node* node = getNodeByPath(path);
        if (node) {
            std::cout << "Size for " << path << ": " << node->length << std::endl;
            return node->length;  // Return the length of the content
        }
    }
    std::cout << "Node not found or is a directory for path: " << path << std::endl;
    return -1;
}

// Method to read contents of a file
int Wad::getContents(const std::string &path, char *buffer, int length, int offset) {
    if (!isContent(path)) {
        return -1; // Return -1 if it's not a content file
    }

    Node* node = getNodeByPath(path);
    if (!node) {
        return -1; // Path does not exist
    }

    std::ifstream file(wadPath, std::ios::binary); // Replace with the actual WAD file path
    if (!file) {
        return -1; // Error opening file
    }

    file.seekg(node->offset + offset); // Move to the correct position in the file
    int actualLength = std::min(node->length - offset, length); // Calculate the actual number of bytes to read
    if (actualLength > 0) {
        file.read(buffer, actualLength); // Read data into the buffer
    } else {
        actualLength = 0; // No data to read
    }

    return actualLength;
}

// Method to read a directory's contents
int Wad::getDirectory(const std::string &path, std::vector<std::string> *directory) {
    std::cout << "getDirectory called with path: " << path << std::endl;

    if (!isDirectory(path)) {
        std::cout << "Path is not a directory: " << path << std::endl;
        return -1; // Return -1 if the path does not represent a directory
    }

    Node* dirNode = getNodeByPath(path);
    if (!dirNode) {
        std::cout << "Directory node not found for path: " << path << std::endl;
        return -1; // Directory node not found
    }

    directory->clear(); // Clear any existing contents in the directory vector
    for (auto child : dirNode->children) {
        directory->push_back(child->name); // Add the name of each child to the directory vector
    }

    std::cout << "Number of children in directory: " << directory->size() << std::endl;

    return directory->size();
}

/* FILE CREATION */

std::string Wad::extractDirectoryName(const std::string& path) {
    if (path.back() == '/') {
        // If path ends with '/', remove it before extracting the name
        auto pos = path.find_last_of('/', path.length() - 2);
        return path.substr(pos + 1, path.length() - pos - 2);
    } else {
        auto pos = path.find_last_of('/');
        return path.substr(pos + 1);
    }
}

std::string Wad::extractParentPath(const std::string& path) {
    if (path == "/") {
        // Root directory is its own parent
        return "/";
    }

    // Normalize the path by removing a trailing slash if present
    std::string normalizedPath = path;
    if (normalizedPath.back() == '/') {
        normalizedPath.pop_back();
    }

    // Find the last slash in the normalized path
    auto pos = normalizedPath.find_last_of('/');
    if (pos == std::string::npos) {
        // If no slash found, the parent is root
        return "/";
    }

    // Extract the parent path up to but not including the last slash
    return normalizedPath.substr(0, pos);
}

std::string Wad::extractParentPathForFiles(const std::string& path) {
    if (path == "/") {
        // Root directory is its own parent
        return "/";
    }

    // Normalize the path by removing a trailing slash if present
    std::string normalizedPath = path;
    // if (normalizedPath.back() == '/') {
    //     normalizedPath.pop_back();
    // }

    // Find the last slash in the normalized path
    auto pos = normalizedPath.find_last_of('/');
    if (pos == std::string::npos) {
        // If no slash found, the parent is root
        return "/";
    }

    // Extract the parent path up to but not including the last slash
    return normalizedPath.substr(0, pos);
}

void Wad::insertDescriptors(Node* parentNode, Descriptor& startDescriptor, Descriptor& endDescriptor) {
if (parentNode == root) {
        // If the parent is the root directory, insert the new descriptors at the end
        descriptors.push_back(startDescriptor);
        descriptors.push_back(endDescriptor);
        numDescriptors += 2;
    } else {
        // For non-root parent directories, find the "_END" descriptor and insert before it
        auto it = std::find_if(descriptors.begin(), descriptors.end(),
                               [parentNode](const Descriptor& desc) {
                                   return desc.descriptorName == parentNode->name + "_END";
                               });

        if (it != descriptors.end()) {
            descriptors.insert(it, endDescriptor);
            descriptors.insert(it, startDescriptor);
            numDescriptors += 2;
        }
    }
}

void Wad::insertDescriptorForFiles(Node* parentNode, Descriptor& fileDescriptor) {
if (parentNode == root) {
        // If the parent is the root directory, insert the new descriptors at the end
        descriptors.push_back(fileDescriptor);
        numDescriptors += 1;
    } else {
        // For non-root parent directories, find the "_END" descriptor and insert before it
        auto it = std::find_if(descriptors.begin(), descriptors.end(),
                               [parentNode](const Descriptor& desc) {
                                   return desc.descriptorName == parentNode->name + "_END";
                               });

        if (it != descriptors.end()) {
            descriptors.insert(it, fileDescriptor);
            numDescriptors += 1;
        }
    }
}


void Wad::addDirectoryToTreeAndMap(const std::string& dirName, Node* parentNode, const std::string& fullPath) {
    // Print the tree and path map before adding the new directory
    std::cout << "Tree before adding new directory:\n";
    printTree(root);
    std::cout << "\nPath map before adding new directory:\n";
    for (const auto& pair : pathMap) {
        std::cout << "Path: " << pair.first << ", Node Name: " << pair.second->name << "\n";
    }

    // Add the new directory to the tree and map
    Node* newDir = new Node{dirName, true, {}, 0, 0, parentNode};
    parentNode->children.push_back(newDir);
    addToPathMap(fullPath, newDir);

    // Print the tree and path map after adding the new directory
    std::cout << "\nTree after adding new directory:\n";
    printTree(root);
    std::cout << "\nPath map after adding new directory:\n";
    for (const auto& pair : pathMap) {
        std::cout << "Path: " << pair.first << ", Node Name: " << pair.second->name << "\n";
    }
}

void Wad::addFileToTreeAndMap(const std::string& dirName, Node* parentNode, const std::string& fullPath) {
    // Print the tree and path map before adding the new directory
    std::cout << "Tree before adding new directory:\n";
    printTree(root);
    std::cout << "\nPath map before adding new directory:\n";
    for (const auto& pair : pathMap) {
        std::cout << "Path: " << pair.first << ", Node Name: " << pair.second->name << "\n";
    }

    // Add the new directory to the tree and map
    Node* newDir = new Node{dirName, true, {}, 0, 0, parentNode};
    newDir->isDirectory = false;
    parentNode->children.push_back(newDir);
    addToPathMap(fullPath, newDir);

    // Print the tree and path map after adding the new directory
    std::cout << "\nTree after adding new directory:\n";
    printTree(root);
    std::cout << "\nPath map after adding new directory:\n";
    for (const auto& pair : pathMap) {
        std::cout << "Path: " << pair.first << ", Node Name: " << pair.second->name << "\n";
    }
}


void Wad::updateHeader() {
    std::fstream wadFile(wadPath, std::ios::in | std::ios::out | std::ios::binary);
    if (!wadFile) {
        throw std::runtime_error("Failed to open WAD file for updating header.");
    }

    // logging before
    std::cout << "Current number of descriptors (before update): " << numDescriptors << std::endl;

    // CAUSED ERRORS
    //numDescriptors += 2;

    // logging number
    std::cout << "Updating WAD file header. New number of descriptors: " << numDescriptors << std::endl;

    // seek position in file
    wadFile.seekp(4, std::ios::beg);
    wadFile.write(reinterpret_cast<const char*>(&numDescriptors), sizeof(numDescriptors));

    if (!wadFile) {
        throw std::runtime_error("Failed to write updated number of descriptors to WAD file.");
    }

    wadFile.close();

    std::cout << "Header updated with new number of descriptors." << std::endl;
}

void Wad::writeDescriptor(std::fstream& wadFile, const Descriptor& descriptor) {
    wadFile.write(reinterpret_cast<const char*>(&descriptor.elementOffset), sizeof(descriptor.elementOffset));
    wadFile.write(reinterpret_cast<const char*>(&descriptor.elementLength), sizeof(descriptor.elementLength));
    wadFile.write(descriptor.descriptorName.c_str(), 8);
}

void Wad::writeUpdatedDescriptorsToFile() {
    std::cout << "Descriptors before writing:" << std::endl;
    for (const auto& descriptor : descriptors) {
        std::cout << "Name: " << descriptor.descriptorName << ", Offset: " << descriptor.elementOffset << ", Length: " << descriptor.elementLength << std::endl;
    }

    std::fstream wadFile(wadPath, std::ios::in | std::ios::out | std::ios::binary);
    if (!wadFile) {
        throw std::runtime_error("Failed to open WAD file for writing.");
    }

    // start of desclist
    wadFile.seekp(descriptorOffset);

    // writing to file
    for (const auto& descriptor : descriptors) {
        wadFile.write(reinterpret_cast<const char*>(&descriptor.elementOffset), sizeof(descriptor.elementOffset));
        wadFile.write(reinterpret_cast<const char*>(&descriptor.elementLength), sizeof(descriptor.elementLength));

        // write descr name
        std::string paddedName = descriptor.descriptorName;
        paddedName.resize(8, '\0');
        wadFile.write(paddedName.c_str(), 8);
    }

    wadFile.close();

    std::cout << "Descriptors after writing:" << std::endl;
    for (const auto& descriptor : descriptors) {
        std::cout << "Name: " << descriptor.descriptorName << ", Offset: " << descriptor.elementOffset << ", Length: " << descriptor.elementLength << std::endl;
    }
}

Node* Wad::getNodeForDirectoryCreation(const std::string &path) {
    // for root case
    if (path.empty() || path == "/") {
        return root;
    }

    std::string modifiedPath = path;

    // remove last char if slash
    if (!modifiedPath.empty() && modifiedPath.back() == '/') {
        modifiedPath.pop_back();
    }

    std::cout << "path before searching for the node via path: " << modifiedPath<< std::endl;

    Node* node = getNodeByPath(modifiedPath);
    if (!node) {
        std::cout << "parentNode not found for path: " << modifiedPath << std::endl;
        return nullptr;
    }
    //std::cout << "here" << std::endl;
    return node;
}

void Wad::createDirectory(const std::string &path){
    std::string dirName = extractDirectoryName(path);
    std::string parentPath = extractParentPath(path);
    std::cout << " directory name: " << dirName << std::endl;
    std::cout << " parent dir name: " << parentPath << std::endl;

    if (dirName.length() > 2){
        return ;
    }

    //FIXED: THIS MAKE SURE TO CHECK IF PARENT NODE IS A MAP DIRECTORY
    if (getNodeForDirectoryCreation(parentPath) == nullptr) {
        return;
    }

    Node* parentNode = getNodeForDirectoryCreation(parentPath);

    std::cout << "parentNode name: " << parentNode->name << std::endl;

    if (!parentNode || !parentNode->isDirectory || isMapMarker(parentNode->name)) {
        return;
    }

    //start n end descriptors for new dir
    Descriptor startDescriptor;
    startDescriptor.elementOffset = 0;
    startDescriptor.elementLength = 0;
    startDescriptor.descriptorName = dirName + "_START";

    Descriptor endDescriptor;
    endDescriptor.elementOffset = 0;
    endDescriptor.elementLength = 0;
    endDescriptor.descriptorName = dirName + "_END";

    insertDescriptors(parentNode, startDescriptor, endDescriptor);
    //create 32 bytes worth of space (16 for each new descriptor) shift the rest of the descriptor list forward so that you can fit the two new descriptors into the list

    //update tree and data structures
    std::string fullPath = parentPath + "/" + dirName;
    addDirectoryToTreeAndMap(dirName, parentNode, fullPath);

    // update the header w new descriptors
    updateHeader();

    // write it back to the file the new descriptors not just the header
    writeUpdatedDescriptorsToFile();
}

void Wad::createFile(const std::string &path){

    std::string fileName = extractDirectoryName(path);
    std::string parentPath = extractParentPathForFiles(path);

    if (fileName.length() > 8) {
        return;
    }

     // Check for illegal sequences in the file name
    if (fileName.find("_START") != std::string::npos || 
        fileName.find("_END") != std::string::npos ||
        isMapMarker(fileName)) {
        std::cout << "Illegal sequence in file name: " << fileName << std::endl;
        return;
    }

    //FIXED: THIS MAKE SURE TO CHECK IF PARENT NODE IS A MAP DIRECTORY
    if (getNodeForDirectoryCreation(parentPath) == nullptr) {
        return;
    }

    Node* parentNode = getNodeForDirectoryCreation(parentPath);

    std::cout << "parentNode name: " << parentNode->name << std::endl;

    if (!parentNode || !parentNode->isDirectory || isMapMarker(parentNode->name)) {
        return;
    }

    // Check if the file already exists
    std::string fullPath = parentPath + "/" + fileName;
    if (getNodeByPath(fullPath) != nullptr) {
        std::cout << "File already exists: " << fullPath << std::endl;
        return;
    }

    // Create a new file descriptor
    Descriptor fileDescriptor;
    fileDescriptor.elementOffset = descriptorOffset;
    fileDescriptor.elementLength = 0;
    std::cout << "ELEMENT LENGTH IN CREATE FILE: " << fileDescriptor.elementLength << std::endl;
    fileDescriptor.descriptorName = fileName;

    // Insert the new file descriptor into the descriptors vector
    //descriptors.push_back(fileDescriptor);
    //numDescriptors++;

    // Insert the start and end descriptors into the parent node
    insertDescriptorForFiles(parentNode, fileDescriptor);

    // Create a new directory node and add it to the tree and path map
    // std::string fullPath = parentPath + "/" + fileName;
    addFileToTreeAndMap(fileName, parentNode, fullPath);

    // Update the descriptor list in the WAD file
    writeUpdatedDescriptorsToFile();

    // Update the header in the WAD file
    updateHeader();

}

int Wad::writeToFile(const std::string &path, const char *buffer, int length, int offset) {

    std::cerr << "Writing to file at path: " << path << std::endl;

    Node* fileNode = getNodeByPath(path);
    if (!fileNode || fileNode->isDirectory ) {
        std::cerr << "Invalid node, is a directory, or already has content." << std::endl;
        return -1;
    }

    Descriptor* descriptorIt = nullptr; // Pointer to hold the found descriptor
    int index = 0;
    for (auto& desc : descriptors) {
        if (desc.descriptorName == fileNode->name) {
            descriptorIt = &desc;
            break; // Stop the loop once the descriptor is found
        }
        index++;
    }

    if (!descriptorIt) {
        std::cerr << "Invalid path or path is a directory, or descriptor not found." << std::endl;
        return -1;
    }

    if (descriptorIt->elementLength != 0) {
        std::cerr << "File is not empty. elementlength: " << descriptorIt->elementLength << std::endl;
        return 0;
    }

    std::fstream wadFile(wadPath, std::ios::binary | std::ios::in | std::ios::out);
    if (!wadFile.is_open()) {
        std::cerr << "Error opening WAD file." << std::endl;
        return -1;
    }

    // Calculate insertion point and check file size
    wadFile.seekg(0, std::ios::end);
    std::streampos fileSize = wadFile.tellg();
    std::streampos dataInsertPoint = descriptorOffset;
    std::streampos newDescriptorOffset = descriptorOffset + length;
    
    // Reading existing data from the insertion point to the end of the file
    std::vector<char> existingData(fileSize - dataInsertPoint);
    wadFile.seekg(dataInsertPoint);
    wadFile.read(existingData.data(), existingData.size()); // populates
    //wadFile.flush();

    std::cout << "File size b4 truncate: " << fileSize << std::endl;
    // increase the size of the file by length
    // Expand the file to accommodate new data
    wadFile.seekp(0, std::ios::end);
    for (int i = 0; i < length; ++i) {
        wadFile.put(0);
    }

    std::cout << "File size after truncate: " << wadFile.tellg() << std::endl;

    // then shift existing data by length
    // Shift data to make space for new data
    wadFile.seekp(dataInsertPoint + length);
    wadFile.write(existingData.data(), existingData.size());
    wadFile.flush();

    //uWRITE THE DATA FROM INSERTPOINT OF LENGTH SIZE
    wadFile.seekp(dataInsertPoint);
    wadFile.write(buffer, length);
    wadFile.flush();

    //update offset in header
    wadFile.seekp(8, std::ios::beg);
    wadFile.write(reinterpret_cast<char*>(&newDescriptorOffset), sizeof(newDescriptorOffset));
    wadFile.flush();

    //update file node length and offset before updating tree and map nodes and file itself
    fileNode->offset = static_cast<int>(dataInsertPoint + offset);
    std::cout << "offset is: " << fileNode->offset << std::endl;
    fileNode->length = length; // Update the file node length
    descriptorIt->elementOffset = fileNode->offset;
    descriptorIt->elementLength = fileNode->length;

    //update tree and map node shit. fileNode offset and length
    // update map
    auto pathMapIt = pathMap.find(path);
    if (pathMapIt != pathMap.end()) {
        pathMapIt->second->offset = fileNode->offset;
        pathMapIt->second->length = fileNode->length;
        std::cout << "FILENODE LENGTH IN MAP: " << pathMapIt->second->length << std::endl;
    }

    
    printTree(root, 0);
    // updateNodeInTree(root, path, fileNode->offset, fileNode->length);

    //update the filenode offset and length in the actual file by iterating thru descriptors
    //find descriptor in file and update offset node and length

    // Update the specific descriptor in the file
    int descriptorFilePosition = descriptorOffset + index * 16 + length;
    std::cout << "location in file: " << descriptorFilePosition << std::endl;
    wadFile.seekp(descriptorFilePosition, std::ios::beg);
    wadFile.write(reinterpret_cast<const char*>(&descriptorIt->elementOffset), sizeof(descriptorIt->elementOffset));
    wadFile.flush();

    wadFile.seekp(descriptorFilePosition + 4, std::ios::beg);
    wadFile.write(reinterpret_cast<const char*>(&descriptorIt->elementLength), sizeof(descriptorIt->elementLength));
    wadFile.flush();

    // Seek to the file offset
    wadFile.seekg(fileNode->offset);

    // Read the file contents into a buffer
    std::vector<char> buffa(fileNode->length);
    wadFile.read(buffa.data(), buffa.size());

    return length;
}
