#include <iostream>
#include <string>
#include <vector>
#include <memory>
using namespace std;

struct IFileSystemNode {
    virtual int getSize() const = 0;
    virtual string getName() const = 0;
    virtual void tree() const = 0;
    virtual ~IFileSystemNode() = default;
};

class File : public IFileSystemNode {
    string name, content;
public:
    File(string name, string content) : name(move(name)), content(move(content)) {}

    int getSize() const override { return (int)content.size(); }
    string getName() const override { return name; }
    void tree() const override { cout << "File " << name << "\n"; }
};

class Folder : public IFileSystemNode {
    string name;
    vector<unique_ptr<IFileSystemNode>> children;
public:
    explicit Folder(string name) : name(move(name)) {}

    void touch(const string& n, const string& content) {
        children.push_back(make_unique<File>(n, content));
    }

    void mkdir(const string& n) {
        children.push_back(make_unique<Folder>(n));
    }

    int getSize() const override {
        int size = 0;
        for (const auto& child : children) size += child->getSize();
        return size;
    }

    string getName() const override { return name; }

    void tree() const override {
        cout << "Folder:-> " << name << "\n";
        for (const auto& child : children) child->tree();
    }
};

int main() {
    Folder root("root");
    root.touch("System.txt", "dasdsadasd");
    root.mkdir("Doc");
    return 0;
}
