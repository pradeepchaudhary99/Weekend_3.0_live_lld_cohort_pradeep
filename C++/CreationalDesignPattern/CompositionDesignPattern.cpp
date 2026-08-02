#include <iostream>
#include <memory>
#include <string>
#include <vector>

struct IFileSystemNode {
    virtual int getSize() = 0;
    virtual std::string getName() = 0;
    virtual void tree() = 0;
    virtual ~IFileSystemNode() = default;
};

class File : public IFileSystemNode {
    std::string name;
    std::string content;

public:
    File(std::string name, std::string content)
        : name(std::move(name)), content(std::move(content)) {}

    int getSize() override { return static_cast<int>(content.length()); }

    std::string getName() override { return name; }

    void tree() override { std::cout << "File " << name << std::endl; }
};

class Folder : public IFileSystemNode {
    std::string name;
    std::vector<std::unique_ptr<IFileSystemNode>> childrens;

public:
    explicit Folder(std::string name) : name(std::move(name)) {}

    void touch(const std::string& name, const std::string& content) {
        childrens.push_back(std::make_unique<File>(name, content));
    }

    void mkdir(const std::string& name) {
        childrens.push_back(std::make_unique<Folder>(name));
    }

    int getSize() override {
        int size = 0;
        for (auto& node : childrens) {
            size += node->getSize();
        }
        return size;
    }

    std::string getName() override { return name; }

    void tree() override {
        std::cout << "Folder:-> " << name << std::endl;
        for (auto& node : childrens) {
            node->tree();
        }
    }
};

class FileSystemManager {
public:
    std::string pathToCd;

    void goToCurrentDirectory() {}
};

int main() {
    // Root
    // System.txt
    // Doc
    // pradeep_lld.txt
    // DSA
    // pradeep_dsa.txt

    Folder root("root");
    root.touch("System.txt", "dasdsadasd");
    root.mkdir("Doc");

    return 0;
}
