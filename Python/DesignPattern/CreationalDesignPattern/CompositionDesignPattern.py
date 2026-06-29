from abc import ABC, abstractmethod
from typing import List


class IFileSystemNode(ABC):
    @abstractmethod
    def get_size(self) -> int: pass

    @abstractmethod
    def get_name(self) -> str: pass

    @abstractmethod
    def tree(self): pass


class File(IFileSystemNode):
    def __init__(self, name: str, content: str):
        self._name = name
        self._content = content

    def get_size(self) -> int:
        return len(self._content)

    def get_name(self) -> str:
        return self._name

    def tree(self):
        print(f"File {self._name}")


class Folder(IFileSystemNode):
    def __init__(self, name: str):
        self._name = name
        self._children: List[IFileSystemNode] = []

    def touch(self, name: str, content: str):
        self._children.append(File(name, content))

    def mkdir(self, name: str):
        self._children.append(Folder(name))

    def get_size(self) -> int:
        return sum(child.get_size() for child in self._children)

    def get_name(self) -> str:
        return self._name

    def tree(self):
        print(f"Folder:-> {self._name}")
        for child in self._children:
            child.tree()


if __name__ == "__main__":
    root = Folder("root")
    root.touch("System.txt", "dasdsadasd")
    root.mkdir("Doc")
