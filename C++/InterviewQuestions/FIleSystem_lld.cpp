/*
File System
------------------------------

Functional Requirements:
    Create files and directories at a path (including arbitrary/nested paths)
    Read/write/delete operations can be performed on files and folders
    Calculate size of a folder/file, list a directory (ls)
    cd / copy / rename / mkdir / getProperties / find / move a file between
    folders / sort files

Non-Functional Requirements:
    Concurrency control: per-node lock so concurrent writers on the same
    file/folder don't corrupt state
    Extensibility: File and Directory share a FileSystemNode base so new
    node types can be added without touching the facade
    Path resolution: paths are resolved one segment at a time from the root
    Access control per folder/per file is out of scope for this demo

Entities:
    FileSystemNode [Abstract]: File, Directory
    FileNode
    DirectoryNode
    FileSystem (Facade)
*/

#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

class DirectoryNode;

class FileSystemNode : public std::enable_shared_from_this<FileSystemNode> {
public:
    FileSystemNode(std::string name, std::shared_ptr<DirectoryNode> parent)
        : name_(std::move(name)), parent_(std::move(parent)),
          createdAt_(std::chrono::system_clock::now()), modifiedAt_(createdAt_) {}

    virtual ~FileSystemNode() = default;

    std::string getName() const { return name_; }

    void setName(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        name_ = name;
        modifiedAt_ = std::chrono::system_clock::now();
    }

    std::shared_ptr<DirectoryNode> getParent() const { return parent_.lock(); }
    void setParent(std::shared_ptr<DirectoryNode> parent) { parent_ = parent; }

    virtual bool isDirectory() const = 0;
    virtual long long getSize() const = 0;
    std::string getPath() const;

protected:
    std::string name_;
    std::weak_ptr<DirectoryNode> parent_;
    std::chrono::system_clock::time_point createdAt_;
    std::chrono::system_clock::time_point modifiedAt_;
    mutable std::mutex mutex_;
};

class FileNode : public FileSystemNode {
public:
    FileNode(std::string name, std::shared_ptr<DirectoryNode> parent)
        : FileSystemNode(std::move(name), std::move(parent)) {}

    bool isDirectory() const override { return false; }

    void write(const std::string& data, bool append) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (append) {
            content_ += data;
        } else {
            content_ = data;
        }
        modifiedAt_ = std::chrono::system_clock::now();
    }

    std::string read() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return content_;
    }

    long long getSize() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return static_cast<long long>(content_.size());
    }

private:
    std::string content_;
};

class DirectoryNode : public FileSystemNode {
public:
    DirectoryNode(std::string name, std::shared_ptr<DirectoryNode> parent)
        : FileSystemNode(std::move(name), std::move(parent)) {}

    bool isDirectory() const override { return true; }

    void addChild(std::shared_ptr<FileSystemNode> node) {
        std::lock_guard<std::mutex> lock(mutex_);
        children_[node->getName()] = std::move(node);
        modifiedAt_ = std::chrono::system_clock::now();
    }

    std::shared_ptr<FileSystemNode> removeChild(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = children_.find(name);
        if (it == children_.end()) return nullptr;
        auto node = it->second;
        children_.erase(it);
        modifiedAt_ = std::chrono::system_clock::now();
        return node;
    }

    std::shared_ptr<FileSystemNode> getChild(const std::string& name) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = children_.find(name);
        return it == children_.end() ? nullptr : it->second;
    }

    std::vector<std::shared_ptr<FileSystemNode>> listChildren() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::shared_ptr<FileSystemNode>> result;
        result.reserve(children_.size());
        for (auto& [name, node] : children_) {
            result.push_back(node);
        }
        return result;
    }

    long long getSize() const override {
        auto children = listChildren();
        long long total = 0;
        for (auto& child : children) total += child->getSize();
        return total;
    }

private:
    std::map<std::string, std::shared_ptr<FileSystemNode>> children_;
};

std::string FileSystemNode::getPath() const {
    auto parent = parent_.lock();
    if (!parent) return "/";
    std::string parentPath = parent->getPath();
    std::string prefix = parentPath == "/" ? "" : parentPath;
    return prefix + "/" + name_;
}

class FileSystemError : public std::runtime_error {
public:
    explicit FileSystemError(const std::string& message) : std::runtime_error(message) {}
};

// Facade: translates path-based operations into node-graph walks.
class FileSystem {
public:
    FileSystem() : root_(std::make_shared<DirectoryNode>("/", nullptr)), pwd_(root_) {}

    std::shared_ptr<FileSystemNode> resolve(const std::string& path) const {
        std::shared_ptr<FileSystemNode> current = path.empty() || path.front() == '/' ? root_ : pwd_;
        for (auto& token : tokenize(path)) {
            auto directory = std::dynamic_pointer_cast<DirectoryNode>(current);
            if (token == "." || !directory) continue;
            if (token == "..") {
                current = current->getParent() ? current->getParent() : root_;
                continue;
            }
            auto child = directory->getChild(token);
            if (!child) return nullptr;
            current = child;
        }
        return current;
    }

    std::pair<std::shared_ptr<DirectoryNode>, std::string> resolveParentDirectory(const std::string& path) const {
        auto tokens = tokenize(path);
        if (tokens.empty()) throw FileSystemError("Invalid path: " + path);
        std::string leafName = tokens.back();
        tokens.pop_back();

        std::shared_ptr<FileSystemNode> parent;
        if (tokens.empty()) {
            parent = (!path.empty() && path.front() == '/') ? root_ : pwd_;
        } else {
            std::string parentPath = (!path.empty() && path.front() == '/' ? "/" : "") + join(tokens);
            parent = resolve(parentPath);
        }

        auto directory = std::dynamic_pointer_cast<DirectoryNode>(parent);
        if (!directory) throw FileSystemError("Parent directory does not exist for path: " + path);
        return {directory, leafName};
    }

    // Creates every missing directory along the path, like `mkdir -p`.
    std::shared_ptr<DirectoryNode> mkdir(const std::string& path) {
        std::shared_ptr<DirectoryNode> current = (!path.empty() && path.front() == '/') ? root_ : pwd_;
        for (auto& token : tokenize(path)) {
            auto child = current->getChild(token);
            if (!child) {
                auto newDir = std::make_shared<DirectoryNode>(token, current);
                current->addChild(newDir);
                current = newDir;
            } else if (auto directory = std::dynamic_pointer_cast<DirectoryNode>(child)) {
                current = directory;
            } else {
                throw FileSystemError("'" + token + "' exists and is not a directory");
            }
        }
        return current;
    }

    std::shared_ptr<FileNode> touch(const std::string& path) {
        auto [parent, name] = resolveParentDirectory(path);
        if (parent->getChild(name)) throw FileSystemError("'" + path + "' already exists");
        auto fileNode = std::make_shared<FileNode>(name, parent);
        parent->addChild(fileNode);
        return fileNode;
    }

    void writeFile(const std::string& path, const std::string& data, bool append = false) {
        auto node = resolve(path);
        if (!node) node = touch(path);
        auto fileNode = std::dynamic_pointer_cast<FileNode>(node);
        if (!fileNode) throw FileSystemError("'" + path + "' is not a file");
        fileNode->write(data, append);
    }

    std::string readFile(const std::string& path) const {
        auto fileNode = std::dynamic_pointer_cast<FileNode>(resolve(path));
        if (!fileNode) throw FileSystemError("'" + path + "' is not a file");
        return fileNode->read();
    }

    void deleteNode(const std::string& path) {
        auto [parent, name] = resolveParentDirectory(path);
        if (!parent->removeChild(name)) throw FileSystemError("'" + path + "' does not exist");
    }

    std::vector<std::string> list(const std::string& path = ".") const {
        auto directory = std::dynamic_pointer_cast<DirectoryNode>(resolve(path));
        if (!directory) throw FileSystemError("'" + path + "' is not a directory");
        std::vector<std::string> names;
        for (auto& child : directory->listChildren()) names.push_back(child->getName());
        std::sort(names.begin(), names.end());
        return names;
    }

    void cd(const std::string& path) {
        auto directory = std::dynamic_pointer_cast<DirectoryNode>(resolve(path));
        if (!directory) throw FileSystemError("'" + path + "' is not a directory");
        pwd_ = directory;
    }

    void move(const std::string& sourcePath, const std::string& destinationPath) {
        auto [sourceParent, sourceName] = resolveParentDirectory(sourcePath);
        auto node = sourceParent->getChild(sourceName);
        if (!node) throw FileSystemError("'" + sourcePath + "' does not exist");

        auto destination = resolve(destinationPath);
        std::shared_ptr<DirectoryNode> targetParent;
        std::string targetName;
        if (auto destDir = std::dynamic_pointer_cast<DirectoryNode>(destination)) {
            targetParent = destDir;
            targetName = node->getName();
        } else {
            std::tie(targetParent, targetName) = resolveParentDirectory(destinationPath);
        }

        sourceParent->removeChild(sourceName);
        node->setName(targetName);
        node->setParent(targetParent);
        targetParent->addChild(node);
    }

    // Depth-first search for every node named `name`, returning full paths.
    std::vector<std::string> search(const std::string& name) const {
        std::vector<std::string> matches;
        std::function<void(const std::shared_ptr<FileSystemNode>&)> walk =
            [&](const std::shared_ptr<FileSystemNode>& node) {
                if (node->getName() == name) matches.push_back(node->getPath());
                if (auto directory = std::dynamic_pointer_cast<DirectoryNode>(node)) {
                    for (auto& child : directory->listChildren()) walk(child);
                }
            };
        walk(root_);
        return matches;
    }

    std::shared_ptr<DirectoryNode> root() const { return root_; }

private:
    static std::vector<std::string> tokenize(const std::string& path) {
        std::vector<std::string> tokens;
        std::stringstream stream(path);
        std::string token;
        while (std::getline(stream, token, '/')) {
            if (!token.empty()) tokens.push_back(token);
        }
        return tokens;
    }

    static std::string join(const std::vector<std::string>& tokens) {
        std::string result;
        for (size_t i = 0; i < tokens.size(); ++i) {
            if (i > 0) result += "/";
            result += tokens[i];
        }
        return result;
    }

    std::shared_ptr<DirectoryNode> root_;
    std::shared_ptr<DirectoryNode> pwd_;
};

static void printVector(const std::string& label, const std::vector<std::string>& values) {
    std::cout << label << " [";
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << values[i];
    }
    std::cout << "]\n";
}

int main() {
    FileSystem fs;

    fs.mkdir("/home/alice/docs");
    fs.writeFile("/home/alice/docs/todo.txt", "buy milk\n");
    fs.writeFile("/home/alice/docs/todo.txt", "walk the dog\n", true);
    fs.touch("/home/alice/notes.txt");
    fs.writeFile("/home/alice/notes.txt", "meeting at 5pm");

    printVector("Contents of /home/alice:", fs.list("/home/alice"));
    std::cout << "todo.txt contents:\n" << fs.readFile("/home/alice/docs/todo.txt") << "\n";

    std::cout << "Size of /home/alice: " << fs.resolve("/home/alice")->getSize() << " bytes\n";

    fs.cd("/home/alice");
    printVector("pwd listing (relative path 'docs'):", fs.list("docs"));

    fs.move("/home/alice/notes.txt", "/home/alice/docs/notes.txt");
    printVector("After move, /home/alice/docs:", fs.list("/home/alice/docs"));

    printVector("Search for 'notes.txt':", fs.search("notes.txt"));

    fs.deleteNode("/home/alice/docs/todo.txt");
    printVector("After delete, /home/alice/docs:", fs.list("/home/alice/docs"));

    return 0;
}
