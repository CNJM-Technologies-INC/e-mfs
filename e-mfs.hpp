#pragma once

/**
 * @file e-mfs.hpp
 * @brief Easy Memory File System (E-MFS) - A header-only, in-memory file system with a shell-like C++ API.
 * @details Provides a simple, intuitive interface mimicking shell commands (mkdir, ls, cp, mv, rm, cat, size, etc.)
 *          for managing a file system in memory. Supports binary content, command aliases (dir, del), and
 *          cross-platform file execution.
 * @author Camresh James
 * @date 28-08-2025
 * @version 2.1
 */

#include <algorithm>
#include <cstdio> // For std::remove
#include <cstdlib> // For std::system
#include <filesystem> // For std::filesystem::temp_directory_path
#include <fstream> // <--- FIX: Added for std::ofstream
#include <memory>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#if defined(__linux__) || defined(__APPLE__) || defined(__MACH__)
#include <sys/stat.h> // For chmod
#endif

namespace e_mfs {

// --- Custom Exception Class ---
/**
 * @class FileSystemException
 * @brief Custom exception for file system operations.
 */
class FileSystemException : public std::runtime_error {
public:
    explicit FileSystemException(const std::string& message)
        : std::runtime_error(message) {}
};

// --- Node Type Enumeration ---
enum class NodeType { File, Directory };

// --- Forward Declarations ---
struct FSNode;
struct FileNode;
struct DirectoryNode;

// --- Base Node Structure ---
/**
 * @struct FSNode
 * @brief Abstract base class for file system nodes (files or directories).
 */
struct FSNode {
    std::string name;                     // Name of the node
    std::weak_ptr<DirectoryNode> parent;  // Weak pointer to parent directory to avoid circular references

    FSNode(std::string name, std::shared_ptr<DirectoryNode> parent)
        : name(std::move(name)), parent(std::move(parent)) {}
    virtual ~FSNode() = default;
    virtual NodeType getType() const = 0;
    virtual size_t size() const = 0; // Get the size in bytes
};

// --- Directory Node ---
/**
 * @struct DirectoryNode
 * @brief Represents a directory in the memory file system.
 */
struct DirectoryNode final : public FSNode, public std::enable_shared_from_this<DirectoryNode> {
    std::unordered_map<std::string, std::shared_ptr<FSNode>> children; // Child nodes

    DirectoryNode(const std::string& name, std::shared_ptr<DirectoryNode> parent)
        : FSNode(name, std::move(parent)) {}

    NodeType getType() const override { return NodeType::Directory; }

    /**
     * @brief Recursively calculates the total size of all files within this directory.
     * @return Total size in bytes.
     */
    size_t size() const override {
        size_t total_size = 0;
        for (const auto& [name, child] : children) {
            total_size += child->size();
        }
        return total_size;
    }
};

// --- File Node ---
/**
 * @struct FileNode
 * @brief Represents a file in the memory file system.
 */
struct FileNode final : public FSNode {
    std::vector<char> content; // File content as binary data

    FileNode(const std::string& name, std::shared_ptr<DirectoryNode> parent)
        : FSNode(name, std::move(parent)) {}
    NodeType getType() const override { return NodeType::File; }

    /**
     * @brief Returns the size of the file content.
     * @return Size in bytes.
     */
    size_t size() const override { return content.size(); }
};

// --- Main File System Class ---
/**
 * @class FileSystem
 * @brief Provides a shell-like API for managing an in-memory file system.
 */
class FileSystem {
private:
    std::shared_ptr<DirectoryNode> root; // Root directory of the file system

    // --- Helper Methods ---
    std::shared_ptr<FSNode> _resolvePath(std::string_view path) const {
        if (path.empty()) {
            throw FileSystemException("Path cannot be empty.");
        }
        if (path == "/") return root;

        std::string path_str(path);
        std::stringstream ss(path_str);
        std::string component;
        std::shared_ptr<DirectoryNode> current = root;

        if (ss.peek() == '/') ss.ignore();

        while (std::getline(ss, component, '/')) {
            if (component.empty() || component == ".") continue;
            if (component == "..") {
                if (current != root) {
                    current = current->parent.lock();
                }
                continue;
            }
            auto it = current->children.find(component);
            if (it == current->children.end()) {
                throw FileSystemException("Path not found: " + path_str);
            }
            if (it->second->getType() != NodeType::Directory && ss.peek() != EOF) {
                 throw FileSystemException("Path component is not a directory: " + component);
            }
            if (it->second->getType() == NodeType::Directory) {
                current = std::static_pointer_cast<DirectoryNode>(it->second);
            } else {
                 return it->second; // Return the file if it's the last component
            }
        }
        return current;
    }

    std::pair<std::shared_ptr<DirectoryNode>, std::string> _resolveParentAndName(std::string_view path) const {
        if (path.empty() || path == "/") {
            throw FileSystemException("Invalid path for child creation: " + std::string(path));
        }
        size_t last_slash = path.find_last_of('/');
        if (last_slash == std::string::npos) { // e.g., "file.txt"
            throw FileSystemException("Paths must be absolute (start with '/'): " + std::string(path));
        }
        
        std::string_view parentPath_sv = (last_slash == 0) ? "/" : path.substr(0, last_slash);
        std::string_view childName_sv = path.substr(last_slash + 1);
        
        if (childName_sv.empty()) {
            throw FileSystemException("Path cannot end with a slash for this operation: " + std::string(path));
        }

        auto parentNode = _resolvePath(parentPath_sv);
        if (parentNode->getType() != NodeType::Directory) {
            throw FileSystemException("Parent path is not a directory: " + std::string(parentPath_sv));
        }
        return {std::static_pointer_cast<DirectoryNode>(parentNode), std::string(childName_sv)};
    }
    
    // Improved helper to handle destinations like `cp file /dir/`
    std::pair<std::shared_ptr<DirectoryNode>, std::string> _resolveDestination(std::string_view dest_path, const std::string& source_name) const {
        try {
            auto dest_node = _resolvePath(dest_path);
            if (dest_node->getType() == NodeType::Directory) {
                // Destination is an existing directory, so the final name is the source name.
                auto dest_dir = std::static_pointer_cast<DirectoryNode>(dest_node);
                if (dest_dir->children.count(source_name)) {
                     throw FileSystemException("Destination '" + std::string(dest_path) + "/" + source_name + "' already exists.");
                }
                return {dest_dir, source_name};
            } else {
                // Destination exists and is a file.
                throw FileSystemException("Destination file already exists: " + std::string(dest_path));
            }
        } catch (const FileSystemException&) {
            // Destination path does not fully exist, resolve its parent.
            return _resolveParentAndName(dest_path);
        }
    }


    void _recursiveCopy(const std::shared_ptr<DirectoryNode>& source, std::shared_ptr<DirectoryNode>& dest) const {
        for (const auto& [name, child] : source->children) {
            if (child->getType() == NodeType::File) {
                auto oldFile = std::static_pointer_cast<FileNode>(child);
                auto newFile = std::make_shared<FileNode>(oldFile->name, dest);
                newFile->content = oldFile->content;
                dest->children[newFile->name] = newFile;
            } else {
                auto oldDir = std::static_pointer_cast<DirectoryNode>(child);
                auto newDir = std::make_shared<DirectoryNode>(oldDir->name, dest);
                dest->children[newDir->name] = newDir;
                _recursiveCopy(oldDir, newDir);
            }
        }
    }

public:
    FileSystem() : root(std::make_shared<DirectoryNode>("/", nullptr)) {}

    // --- Core API ---
    void mkdir(std::string_view path) {
        if (path == "/") return;

        std::string path_str(path);
        std::stringstream ss(path_str);
        std::string component;
        auto current = root;

        if (ss.peek() == '/') ss.ignore();

        while (std::getline(ss, component, '/')) {
            if (component.empty()) continue;
            auto it = current->children.find(component);
            if (it == current->children.end()) {
                auto newDir = std::make_shared<DirectoryNode>(component, current);
                current->children[component] = newDir;
                current = newDir;
            } else {
                if (it->second->getType() != NodeType::Directory) {
                    throw FileSystemException("A file exists at path component: " + component);
                }
                current = std::static_pointer_cast<DirectoryNode>(it->second);
            }
        }
    }

    void touch(std::string_view path) {
        auto [parent, fileName] = _resolveParentAndName(path);
        if (parent->children.count(fileName)) {
            // In unix, touch updates timestamp. Here we just ensure it's a file.
            if(parent->children.at(fileName)->getType() != NodeType::File) {
                 throw FileSystemException("Cannot touch '" + std::string(path) + "', a directory with that name exists.");
            }
            return; // File already exists, do nothing.
        }
        parent->children[fileName] = std::make_shared<FileNode>(fileName, parent);
    }

    void writeFile(std::string_view path, const std::vector<char>& content) {
        auto [parent, fileName] = _resolveParentAndName(path);
        if (parent->children.count(fileName) && parent->children.at(fileName)->getType() == NodeType::Directory) {
            throw FileSystemException("Cannot write to '" + fileName + "', it is a directory.");
        }
        auto file = std::make_shared<FileNode>(fileName, parent);
        file->content = content;
        parent->children[fileName] = file;
    }
    
    void writeFile(std::string_view path, std::string_view content) {
        writeFile(path, std::vector<char>(content.begin(), content.end()));
    }

    void append(std::string_view path, const std::vector<char>& content) {
        auto node = _resolvePath(path);
        if (node->getType() != NodeType::File) {
            throw FileSystemException("Path is not a file: " + std::string(path));
        }
        auto file = std::static_pointer_cast<FileNode>(node);
        file->content.insert(file->content.end(), content.begin(), content.end());
    }

    void append(std::string_view path, std::string_view content) {
        append(path, std::vector<char>(content.begin(), content.end()));
    }

    std::vector<char> cat(std::string_view path) const {
        auto node = _resolvePath(path);
        if (node->getType() != NodeType::File) {
            throw FileSystemException("Path is not a file: " + std::string(path));
        }
        return std::static_pointer_cast<FileNode>(node)->content;
    }

    std::string catAsString(std::string_view path) const {
        auto content = cat(path);
        return std::string(content.begin(), content.end());
    }

    void rm(std::string_view path, bool recursive = false) {
        if (path == "/") throw FileSystemException("Cannot remove the root directory.");
        auto [parent, name] = _resolveParentAndName(path);
        auto it = parent->children.find(name);
        if (it == parent->children.end()) {
            throw FileSystemException("Path not found: " + std::string(path));
        }
        if (it->second->getType() == NodeType::Directory && !recursive) {
            if (!std::static_pointer_cast<DirectoryNode>(it->second)->children.empty()) {
                throw FileSystemException("Directory not empty, use recursive flag: " + std::string(path));
            }
        }
        parent->children.erase(it);
    }

    void cp(std::string_view sourcePath, std::string_view destPath) {
        auto sourceNode = _resolvePath(sourcePath);
        auto [destParent, newName] = _resolveDestination(destPath, sourceNode->name);

        if (destParent->children.count(newName)) {
            throw FileSystemException("Destination already exists: " + std::string(destPath) + "/" + newName);
        }

        if (sourceNode->getType() == NodeType::File) {
            auto oldFile = std::static_pointer_cast<FileNode>(sourceNode);
            auto newFile = std::make_shared<FileNode>(newName, destParent);
            newFile->content = oldFile->content;
            destParent->children[newName] = newFile;
        } else {
            auto oldDir = std::static_pointer_cast<DirectoryNode>(sourceNode);
            auto newDir = std::make_shared<DirectoryNode>(newName, destParent);
            destParent->children[newName] = newDir;
            _recursiveCopy(oldDir, newDir);
        }
    }

    void mv(std::string_view sourcePath, std::string_view destPath) {
        if (sourcePath == "/") throw FileSystemException("Cannot move the root directory.");
        auto sourceNode = _resolvePath(sourcePath);
        auto oldParent = sourceNode->parent.lock();
        if (!oldParent) throw FileSystemException("Internal error: source node has no parent.");
        
        auto [newParent, newName] = _resolveDestination(destPath, sourceNode->name);
        
        // Check for moving a directory into itself
        auto tempParent = newParent;
        while(tempParent) {
            if (tempParent.get() == sourceNode.get()) {
                throw FileSystemException("Cannot move a directory into itself.");
            }
            tempParent = tempParent->parent.lock();
        }

        oldParent->children.erase(sourceNode->name); // Erase by old name before rename
        sourceNode->parent = newParent;
        sourceNode->name = newName;
        newParent->children[newName] = sourceNode;
    }

    std::vector<std::string> ls(std::string_view path) const {
        auto node = _resolvePath(path);
        if (node->getType() != NodeType::Directory) {
            throw FileSystemException("Path is not a directory: " + std::string(path));
        }
        std::vector<std::string> entries;
        auto dirNode = std::static_pointer_cast<DirectoryNode>(node);
        for (const auto& [name, child] : dirNode->children) {
            entries.push_back(name + (child->getType() == NodeType::Directory ? "/" : ""));
        }
        std::sort(entries.begin(), entries.end());
        return entries;
    }

    bool exists(std::string_view path) const noexcept {
        try {
            _resolvePath(path);
            return true;
        } catch (...) {
            return false;
        }
    }

    NodeType getNodeType(std::string_view path) const {
        return _resolvePath(path)->getType();
    }
    
    size_t size(std::string_view path) const {
        return _resolvePath(path)->size();
    }
    
    // --- New Features & Aliases ---

    /**
     * @brief [Alias for ls] Lists directory contents.
     */
    std::vector<std::string> dir(std::string_view path) const { return ls(path); }

    /**
     * @brief [Alias for rm] Removes a file or directory.
     */
    void del(std::string_view path, bool recursive = false) { rm(path, recursive); }
    
    /**
     * @brief [Alias for mv] Moves or renames a file or directory.
     */
    void ren(std::string_view sourcePath, std::string_view destPath) { mv(sourcePath, destPath); }

    /**
     * @brief [Alias for catAsString] Reads the content of a file as a string.
     */
    std::string type(std::string_view path) const { return catAsString(path); }

    /**
     * @brief Executes a file from memory.
     * @details Writes the file to a temporary location on the physical disk,
     *          sets executable permissions (on Unix-like systems), runs it, and then cleans up.
     * @param path The absolute path to the executable file in the memory file system.
     * @return The exit code of the executed process.
     * @throws FileSystemException if the path is not a file or on execution failure.
     * @note This operation interacts with the real file system and is platform-dependent.
     */
    int execute(std::string_view path) {
        auto node = _resolvePath(path);
        if (node->getType() != NodeType::File) {
            throw FileSystemException("Path is not a file and cannot be executed: " + std::string(path));
        }

        auto fileNode = std::static_pointer_cast<FileNode>(node);
        std::filesystem::path temp_path = std::filesystem::temp_directory_path();
        
        #if defined(_WIN32) || defined(_WIN64)
            temp_path /= (node->name + ".exe");
            std::string command = "\"" + temp_path.string() + "\"";
        #else
            temp_path /= node->name;
            std::string command = "./" + temp_path.filename().string();
        #endif
        
        // Write to temporary file
        std::ofstream temp_file(temp_path, std::ios::out | std::ios::binary);
        if (!temp_file) {
            throw FileSystemException("Failed to create temporary file for execution.");
        }
        temp_file.write(fileNode->content.data(), fileNode->content.size());
        temp_file.close();

        int result = -1;
        try {
            #if defined(__linux__) || defined(__APPLE__) || defined(__MACH__)
                if (chmod(temp_path.string().c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0) {
                    throw FileSystemException("Failed to set executable permissions on temporary file.");
                }
            #endif
            
            // Execute from the temp directory context to handle relative paths simply
            std::string full_command = "cd \"" + temp_path.parent_path().string() + "\" && " + command;
            result = std::system(full_command.c_str());

        } catch(...) {
            std::remove(temp_path.string().c_str());
            throw; // Re-throw the exception after cleaning up
        }
        
        std::remove(temp_path.string().c_str()); // Cleanup
        return result;
    }
};

} // namespace e_mfs