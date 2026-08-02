from abc import ABC, abstractmethod
from typing import List


class IFileSystemNode(ABC):
    @abstractmethod
    def get_size(self) -> int:
        pass

    @abstractmethod
    def get_name(self) -> str:
        pass

    @abstractmethod
    def tree(self):
        pass


class File(IFileSystemNode):
    def __init__(self, name: str, content: str):
        self.name = name
        self.content = content

    def get_size(self) -> int:
        return len(self.content)

    def get_name(self) -> str:
        return self.name

    def tree(self):
        print(f"File {self.name}")


class Folder(IFileSystemNode):
    def __init__(self, name: str):
        self.name = name
        self.childrens: List[IFileSystemNode] = []

    def touch(self, name: str, content: str):
        self.childrens.append(File(name, content))

    def mkdir(self, name: str):
        self.childrens.append(Folder(name))

    def get_size(self) -> int:
        size = 0
        for node in self.childrens:
            size += node.get_size()
        return size

    def get_name(self) -> str:
        return self.name

    def tree(self):
        print(f"Folder:-> {self.name}")
        for node in self.childrens:
            node.tree()


class FileSystemManager:
    def __init__(self):
        self.path_to_cd = None

    def go_to_current_directory(self):
        pass


def main():
    # Root
    # System.txt
    # Doc
    # pradeep_lld.txt
    # DSA
    # pradeep_dsa.txt

    root = Folder("root")
    root.touch("System.txt", "dasdsadasd")
    root.mkdir("Doc")


if __name__ == "__main__":
    main()
