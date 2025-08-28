/**
 * @file main.cpp
 * @brief Demonstration of the enhanced Easy Memory File System (E-MFS).
 * @details This program showcases advanced features like size calculation, command aliases,
 *          improved move/copy operations, cross-platform execution, and robust error handling.
 * @author Your Name
 * @date 28-08-2025
 */

#include "e-mfs.hpp"
#include <iostream>
#include <vector>
#include <iomanip>

// Helper function to print section headers for clarity
void print_header(const std::string& title) {
    std::cout << "\n--- " << title << " ---\n";
}

// Helper function to print binary content as a hex dump
void print_binary(const std::vector<char>& data) {
    std::cout << "Binary content (hex): ";
    for (char c : data) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << (static_cast<int>(c) & 0xff) << " ";
    }
    std::cout << std::dec << std::endl;
}

int main() {
    e_mfs::FileSystem fs;
    std::cout << "E-MFS v2.0 Demonstration" << std::endl;

    // --- 1. Basic Setup: Directories and Files ---
    print_header("1. Basic Setup");
    try {
        fs.mkdir("/home");
        fs.mkdir("/home/user/documents");
        fs.mkdir("/tmp");
        fs.writeFile("/home/user/notes.txt", "This is a test file in the memory file system.");
        
        std::vector<char> binary_data = {'\xDE', '\xAD', '\xBE', '\xEF'};
        fs.writeFile("/home/user/data.bin", binary_data);
        
        std::cout << "Created directory structure and files." << std::endl;
        auto contents = fs.ls("/home/user");
        std::cout << "Contents of /home/user:" << std::endl;
        for (const auto& item : contents) {
            std::cout << "  - " << item << std::endl;
        }
    } catch (const e_mfs::FileSystemException& e) {
        std::cerr << "Error during setup: " << e.what() << std::endl;
    }

    // --- 2. NEW: Size Calculation ---
    print_header("2. Size Calculation");
    try {
        size_t file_size = fs.size("/home/user/notes.txt");
        size_t bin_size = fs.size("/home/user/data.bin");
        size_t dir_size = fs.size("/home/user");
        std::cout << "Size of '/home/user/notes.txt': " << file_size << " bytes" << std::endl;
        std::cout << "Size of '/home/user/data.bin': " << bin_size << " bytes" << std::endl;
        std::cout << "Total size of '/home/user' (recursive): " << dir_size << " bytes" << std::endl;
    } catch (const e_mfs::FileSystemException& e) {
        std::cerr << "Error calculating size: " << e.what() << std::endl;
    }

    // --- 3. NEW: Command Aliases (dir, ren, type, del) ---
    print_header("3. Command Aliases");
    try {
        std::cout << "Listing '/home/user' with 'dir' command:" << std::endl;
        auto dir_contents = fs.dir("/home/user"); // Alias for ls
        for (const auto& item : dir_contents) {
            std::cout << "  - " << item << std::endl;
        }

        fs.ren("/home/user/notes.txt", "/home/user/renamed_notes.txt"); // Alias for mv
        std::cout << "Renamed notes.txt to renamed_notes.txt." << std::endl;

        std::cout << "Reading renamed file with 'type' command:" << std::endl;
        std::cout << "  > " << fs.type("/home/user/renamed_notes.txt") << std::endl; // Alias for catAsString

        fs.del("/home/user/renamed_notes.txt"); // Alias for rm
        std::cout << "Deleted renamed_notes.txt with 'del'." << std::endl;
        std::cout << "Exists check: " << (fs.exists("/home/user/renamed_notes.txt") ? "Exists" : "Does not exist") << std::endl;
    } catch (const e_mfs::FileSystemException& e) {
        std::cerr << "Error using aliases: " << e.what() << std::endl;
    }

    // --- 4. IMPROVED: `cp` and `mv` into directories ---
    print_header("4. Advanced Copy and Move");
    try {
        fs.writeFile("/tmp/report.log", "Log entry 1.");
        fs.mkdir("/home/user/logs");

        // Copy a file INTO an existing directory
        fs.cp("/tmp/report.log", "/home/user/logs"); 
        std::cout << "Copied /tmp/report.log INTO /home/user/logs/" << std::endl;
        
        // Move a file INTO an existing directory
        fs.mv("/home/user/data.bin", "/home/user/logs");
        std::cout << "Moved /home/user/data.bin INTO /home/user/logs/" << std::endl;

        std::cout << "Contents of /home/user/logs/:" << std::endl;
        for(const auto& item : fs.ls("/home/user/logs")) {
            std::cout << "  - " << item << std::endl;
        }

        std::cout << "Contents of /tmp/ (original copy source should remain):" << std::endl;
         for(const auto& item : fs.ls("/tmp")) {
            std::cout << "  - " << item << std::endl;
        }
    } catch (const e_mfs::FileSystemException& e) {
        std::cerr << "Error in advanced cp/mv: " << e.what() << std::endl;
    }

    // --- 5. NEW: Cross-Platform Execution ---
    print_header("5. Cross-Platform Execution");
    try {
        #if defined(_WIN32) || defined(_WIN64)
            std::string script_content = "@echo off\necho Hello from an in-memory batch script!\ndir";
            std::string script_path = "/home/user/run.bat";
        #else
            std::string script_content = "#!/bin/sh\necho \"Hello from an in-memory shell script!\"\nls -la";
            std::string script_path = "/home/user/run.sh";
        #endif

        fs.writeFile(script_path, script_content);
        std::cout << "Created executable script at '" << script_path << "'" << std::endl;
        std::cout << "Executing script from memory..." << std::endl;
        std::cout << "--- SCRIPT OUTPUT BEGIN ---\n";
        
        int exit_code = fs.execute(script_path);
        
        std::cout << "--- SCRIPT OUTPUT END ---\n";
        std::cout << "Script finished with exit code: " << exit_code << std::endl;

    } catch (const e_mfs::FileSystemException& e) {
        std::cerr << "Error during execution: " << e.what() << std::endl;
    }

    // --- 6. Robust Error Handling ---
    print_header("6. Error Handling");
    fs.mkdir("/tmp/non_empty_dir");
    fs.touch("/tmp/non_empty_dir/some_file.txt");

    std::cout << "Attempting to remove non-empty directory '/tmp/non_empty_dir' without recursive flag..." << std::endl;
    try {
        fs.rm("/tmp/non_empty_dir");
    } catch (const e_mfs::FileSystemException& e) {
        std::cout << "Successfully caught expected exception: " << e.what() << std::endl;
    }

    std::cout << "Now removing with recursive flag..." << std::endl;
    try {
        fs.rm("/tmp/non_empty_dir", true); // Correct usage
        std::cout << "Directory removed successfully." << std::endl;
        std::cout << "Exists check: " << (fs.exists("/tmp/non_empty_dir") ? "Exists" : "Does not exist") << std::endl;
    } catch (const e_mfs::FileSystemException& e) {
        std::cerr << "Caught unexpected exception: " << e.what() << std::endl;
    }

    return 0;
}