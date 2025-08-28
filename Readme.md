# E-MFS: Easy Memory File System

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language](https://img.shields.io/badge/language-C%2B%2B17-blue.svg)](https://isocpp.org/)
[![Header-Only](https://img.shields.io/badge/header--only-yes-brightgreen.svg)]()

E-MFS is a single-header, dependency-free C++17 library that provides a powerful and intuitive in-memory file system. It features a shell-like API that makes managing files and directories in memory as easy as using a terminal.

This library is ideal for applications that need to manage a temporary or virtual file structure, such as for sandboxing, unit testing, managing application resources, or building tools that process file-like data without touching the physical disk.

## Key Features

*   **Shell-like API:** Intuitive methods like `mkdir`, `ls`, `cp`, `mv`, `rm`, and `cat`.
*   **Command Aliases:** Common aliases are included for convenience (`dir`, `del`, `ren`, `type`).
*   **Binary Data Support:** Files can store any `std::vector<char>` content, making it suitable for both text and binary data.
*   **Recursive Operations:** Easily perform recursive directory removal (`rm(path, true)`) and copying.
*   **Size Calculation:** Get the size of a file or the total size of a directory's contents with `fs.size(path)`.
*   **Cross-Platform Execution:** Execute in-memory files on Linux, macOS, and Windows via the `fs.execute(path)` method.
*   **Modern C++:** Built with C++17 features like `std::string_view`, `std::shared_ptr`, and `std::weak_ptr` for performance and safety.
*   **Header-Only:** Just drop `e-mfs.hpp` into your project and include it. No linking required.

## Getting Started

### 1. Include the Header
Simply include the `e-mfs.hpp` file in your project.

```cpp
#include "e-mfs.hpp"
#include <iostream>
```

### 2. Use the API
Instantiate the `FileSystem` class and start using the shell-like commands.

```cpp
int main() {
    e_mfs::FileSystem fs;

    try {
        // 1. Create a directory structure
        fs.mkdir("/home/user/documents");

        // 2. Write a file
        fs.writeFile("/home/user/hello.txt", "Hello, in-memory world!");

        // 3. List the contents of a directory (using an alias)
        std::cout << "Contents of /home/user:" << std::endl;
        for (const auto& item : fs.dir("/home/user")) {
            std::cout << "  - " << item << std::endl;
        }

        // 4. Read the file back (using an alias)
        std::string content = fs.type("/home/user/hello.txt");
        std::cout << "\nFile content: " << content << std::endl;

        // 5. Rename/move the file
        fs.ren("/home/user/hello.txt", "/home/user/documents/world.txt");
        std::cout << "\nMoved file. New contents of /home/user/documents:" << std::endl;
        for (const auto& item : fs.ls("/home/user/documents")) {
            std::cout << "  - " << item << std::endl;
        }

    } catch (const e_mfs::FileSystemException& e) {
        std::cerr << "File system error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
```

## API Overview

| Function         | Aliases      | Description                                               |
| ---------------- | ------------ | --------------------------------------------------------- |
| `mkdir(path)`    |              | Creates a directory, including parent directories.        |
| `touch(path)`    |              | Creates an empty file or does nothing if it exists.       |
| `writeFile(...)` |              | Creates or overwrites a file with content.                |
| `append(...)`    |              | Appends content to an existing file.                      |
| `ls(path)`       | `dir`        | Lists the contents of a directory.                        |
| `cat(path)`      |              | Reads file content as binary `std::vector<char>`.         |
| `catAsString(..)`| `type`       | Reads file content as a `std::string`.                    |
| `rm(path, rec)`  | `del`        | Removes a file or directory (optionally recursive).       |
| `cp(src, dest)`  |              | Copies a file or directory.                               |
| `mv(src, dest)`  | `ren`        | Moves or renames a file or directory.                     |
| `exists(path)`   |              | Checks if a path exists.                                  |
| `size(path)`     |              | Returns the size of a file or total size of a directory.  |
| `execute(path)`  |              | Executes a file from memory (interacts with physical disk). |

## Building

E-MFS requires a **C++17 compliant compiler** (like GCC 8+, Clang 6+, MSVC 19.14+).

To compile a program using E-MFS, simply use the `-std=c++17` flag:
```bash
g++ -std=c++17 your_source_file.cpp -o your_program
```
*Note: Some older versions of GCC (like 8.x) may require linking the filesystem library explicitly with `-lstdc++fs`.*

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.