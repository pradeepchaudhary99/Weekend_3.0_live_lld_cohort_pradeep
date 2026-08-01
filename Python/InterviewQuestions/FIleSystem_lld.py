"""
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
"""

from __future__ import annotations

import threading
import time
from abc import ABC, abstractmethod
from typing import Dict, List, Optional


class FileSystemNode(ABC):
    def __init__(self, name: str, parent: Optional["DirectoryNode"]):
        self._name = name
        self._parent = parent
        self.created_at = time.time()
        self.modified_at = self.created_at
        self._lock = threading.Lock()

    @property
    def lock(self) -> threading.Lock:
        return self._lock

    def get_name(self) -> str:
        return self._name

    def set_name(self, name: str) -> None:
        with self._lock:
            self._name = name
            self.modified_at = time.time()

    def get_parent(self) -> Optional["DirectoryNode"]:
        return self._parent

    def set_parent(self, parent: Optional["DirectoryNode"]) -> None:
        self._parent = parent

    @abstractmethod
    def is_directory(self) -> bool:
        raise NotImplementedError

    @abstractmethod
    def get_size(self) -> int:
        raise NotImplementedError

    def get_path(self) -> str:
        if self._parent is None:
            return "/"
        parent_path = self._parent.get_path()
        prefix = "" if parent_path == "/" else parent_path
        return f"{prefix}/{self._name}"


class FileNode(FileSystemNode):
    def __init__(self, name: str, parent: Optional["DirectoryNode"]):
        super().__init__(name, parent)
        self._content = ""

    def is_directory(self) -> bool:
        return False

    def write(self, data: str, append: bool = False) -> None:
        with self._lock:
            self._content = self._content + data if append else data
            self.modified_at = time.time()

    def read(self) -> str:
        with self._lock:
            return self._content

    def get_size(self) -> int:
        with self._lock:
            return len(self._content)


class DirectoryNode(FileSystemNode):
    def __init__(self, name: str, parent: Optional["DirectoryNode"]):
        super().__init__(name, parent)
        self._children: Dict[str, FileSystemNode] = {}

    def is_directory(self) -> bool:
        return True

    def add_child(self, node: FileSystemNode) -> None:
        with self._lock:
            self._children[node.get_name()] = node
            self.modified_at = time.time()

    def remove_child(self, name: str) -> Optional[FileSystemNode]:
        with self._lock:
            node = self._children.pop(name, None)
            if node is not None:
                self.modified_at = time.time()
            return node

    def get_child(self, name: str) -> Optional[FileSystemNode]:
        with self._lock:
            return self._children.get(name)

    def list_children(self) -> List[FileSystemNode]:
        with self._lock:
            return list(self._children.values())

    def get_size(self) -> int:
        with self._lock:
            children = list(self._children.values())
        return sum(child.get_size() for child in children)


class FileSystemError(Exception):
    pass


class FileSystem:
    """Facade: translates path-based operations into node-graph walks."""

    def __init__(self):
        self.root = DirectoryNode("/", None)
        self.pwd = self.root

    @staticmethod
    def _tokenize(path: str) -> List[str]:
        return [token for token in path.split("/") if token]

    def resolve(self, path: str) -> Optional[FileSystemNode]:
        current: FileSystemNode = self.root if path.startswith("/") else self.pwd
        for token in self._tokenize(path):
            if token == "." or not isinstance(current, DirectoryNode):
                continue
            if token == "..":
                current = current.get_parent() or self.root
                continue
            child = current.get_child(token)
            if child is None:
                return None
            current = child
        return current

    def resolve_parent_directory(self, path: str) -> tuple[DirectoryNode, str]:
        tokens = self._tokenize(path)
        if not tokens:
            raise FileSystemError(f"Invalid path: {path!r}")
        *parent_tokens, leaf_name = tokens
        parent_path = ("/" if path.startswith("/") else "") + "/".join(parent_tokens)
        parent = self.resolve(parent_path) if parent_tokens else (self.root if path.startswith("/") else self.pwd)
        if parent is None or not isinstance(parent, DirectoryNode):
            raise FileSystemError(f"Parent directory does not exist for path: {path!r}")
        return parent, leaf_name

    def mkdir(self, path: str) -> DirectoryNode:
        """Creates every missing directory along the path, like `mkdir -p`."""
        current = self.root if path.startswith("/") else self.pwd
        for token in self._tokenize(path):
            child = current.get_child(token)
            if child is None:
                child = DirectoryNode(token, current)
                current.add_child(child)
            elif not isinstance(child, DirectoryNode):
                raise FileSystemError(f"{token!r} exists and is not a directory")
            current = child
        return current

    def touch(self, path: str) -> FileNode:
        parent, name = self.resolve_parent_directory(path)
        if parent.get_child(name) is not None:
            raise FileSystemError(f"{path!r} already exists")
        file_node = FileNode(name, parent)
        parent.add_child(file_node)
        return file_node

    def write_file(self, path: str, data: str, append: bool = False) -> None:
        node = self.resolve(path)
        if node is None:
            node = self.touch(path)
        if not isinstance(node, FileNode):
            raise FileSystemError(f"{path!r} is not a file")
        node.write(data, append)

    def read_file(self, path: str) -> str:
        node = self.resolve(path)
        if not isinstance(node, FileNode):
            raise FileSystemError(f"{path!r} is not a file")
        return node.read()

    def delete(self, path: str) -> None:
        parent, name = self.resolve_parent_directory(path)
        if parent.remove_child(name) is None:
            raise FileSystemError(f"{path!r} does not exist")

    def list(self, path: str = ".") -> List[str]:
        node = self.resolve(path)
        if not isinstance(node, DirectoryNode):
            raise FileSystemError(f"{path!r} is not a directory")
        return sorted(child.get_name() for child in node.list_children())

    def cd(self, path: str) -> None:
        node = self.resolve(path)
        if not isinstance(node, DirectoryNode):
            raise FileSystemError(f"{path!r} is not a directory")
        self.pwd = node

    def move(self, source_path: str, destination_path: str) -> None:
        source_parent, source_name = self.resolve_parent_directory(source_path)
        node = source_parent.get_child(source_name)
        if node is None:
            raise FileSystemError(f"{source_path!r} does not exist")

        destination = self.resolve(destination_path)
        if isinstance(destination, DirectoryNode):
            target_parent, target_name = destination, node.get_name()
        else:
            target_parent, target_name = self.resolve_parent_directory(destination_path)

        source_parent.remove_child(source_name)
        node.set_name(target_name)
        node.set_parent(target_parent)
        target_parent.add_child(node)

    def search(self, name: str) -> List[str]:
        """Depth-first search for every node named `name`, returning full paths."""
        matches: List[str] = []

        def walk(node: FileSystemNode) -> None:
            if node.get_name() == name:
                matches.append(node.get_path())
            if isinstance(node, DirectoryNode):
                for child in node.list_children():
                    walk(child)

        walk(self.root)
        return matches


def main() -> None:
    fs = FileSystem()

    fs.mkdir("/home/alice/docs")
    fs.write_file("/home/alice/docs/todo.txt", "buy milk\n")
    fs.write_file("/home/alice/docs/todo.txt", "walk the dog\n", append=True)
    fs.touch("/home/alice/notes.txt")
    fs.write_file("/home/alice/notes.txt", "meeting at 5pm")

    print("Contents of /home/alice:", fs.list("/home/alice"))
    print("todo.txt contents:")
    print(fs.read_file("/home/alice/docs/todo.txt"))

    print("Size of /home/alice:", fs.resolve("/home/alice").get_size(), "bytes")

    fs.cd("/home/alice")
    print("pwd listing (relative path 'docs'):", fs.list("docs"))

    fs.move("/home/alice/notes.txt", "/home/alice/docs/notes.txt")
    print("After move, /home/alice/docs:", fs.list("/home/alice/docs"))

    print("Search for 'notes.txt':", fs.search("notes.txt"))

    fs.delete("/home/alice/docs/todo.txt")
    print("After delete, /home/alice/docs:", fs.list("/home/alice/docs"))


if __name__ == "__main__":
    main()
