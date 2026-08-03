/*
    File System

Functional Requirements:
    1. create files and directory at a path or arbitary path
    2. Read/write/delete operations can be performed on files and folders
    3. calculate size of the folder/file, listDirectory (ls)
    4. cd copy/rename/mkdir/ getProperties / find / move file from one folder to another /sort the files

Non-Functional Requirements
    concurrency control
    Extensiblity
    Path Resolution (optimization)
    // Access control per folder/ per file

//Next Step
Identify the Core Entities:
    File, Directory -----<<<< FileSystemNode >>>>>
    FileSystemNode
    FileSystem (Facade)

*/

import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.locks.ReentrantLock;

abstract class FileSystemNode {
    private String name;
    private DirectoryNode parent;
    final long createdAt;
    long modifiedAt;
    final ReentrantLock lock = new ReentrantLock();

    FileSystemNode(String name, DirectoryNode parent) {
        this.name = name;
        this.parent = parent;
        this.createdAt = System.currentTimeMillis();
        this.modifiedAt = createdAt;
    }

    String getName() {
        return name;
    }

    void setName(String name) {
        lock.lock();
        try {
            this.name = name;
            this.modifiedAt = System.currentTimeMillis();
        } finally {
            lock.unlock();
        }
    }

    DirectoryNode getParent() {
        return parent;
    }

    void setParent(DirectoryNode parent) {
        this.parent = parent;
    }

    abstract boolean isDirectory();

    abstract long getSize();

    String getPath() {
        if (parent == null) {
            return "/";
        }
        String parentPath = parent.getPath();
        String prefix = parentPath.equals("/") ? "" : parentPath;
        return prefix + "/" + name;
    }
}

class FileNode extends FileSystemNode {
    private StringBuilder content = new StringBuilder();

    FileNode(String name, DirectoryNode parent) {
        super(name, parent);
    }

    @Override
    boolean isDirectory() {
        return false;
    }

    void write(String data, boolean append) {
        lock.lock();
        try {
            if (append) {
                content.append(data);
            } else {
                content = new StringBuilder(data);
            }
            modifiedAt = System.currentTimeMillis();
        } finally {
            lock.unlock();
        }
    }

    String read() {
        lock.lock();
        try {
            return content.toString();
        } finally {
            lock.unlock();
        }
    }

    @Override
    long getSize() {
        lock.lock();
        try {
            return content.length();
        } finally {
            lock.unlock();
        }
    }
}

class DirectoryNode extends FileSystemNode {
    private final Map<String, FileSystemNode> children = new LinkedHashMap<>();

    DirectoryNode(String name, DirectoryNode parent) {
        super(name, parent);
    }

    @Override
    boolean isDirectory() {
        return true;
    }

    void addChild(FileSystemNode node) {
        lock.lock();
        try {
            children.put(node.getName(), node);
            modifiedAt = System.currentTimeMillis();
        } finally {
            lock.unlock();
        }
    }

    FileSystemNode removeChild(String name) {
        lock.lock();
        try {
            FileSystemNode node = children.remove(name);
            if (node != null) {
                modifiedAt = System.currentTimeMillis();
            }
            return node;
        } finally {
            lock.unlock();
        }
    }

    FileSystemNode getChild(String name) {
        lock.lock();
        try {
            return children.get(name);
        } finally {
            lock.unlock();
        }
    }

    List<FileSystemNode> listChildren() {
        lock.lock();
        try {
            return new ArrayList<>(children.values());
        } finally {
            lock.unlock();
        }
    }

    @Override
    long getSize() {
        long total = 0;
        for (FileSystemNode child : listChildren()) {
            total += child.getSize();
        }
        return total;
    }
}

class FileSystemError extends RuntimeException {
    FileSystemError(String message) {
        super(message);
    }
}

// Facade: translates path-based operations into node-graph walks.
class FileSystem {
    final DirectoryNode root = new DirectoryNode("/", null);
    DirectoryNode pwd = root;

    private static List<String> tokenize(String path) {
        List<String> tokens = new ArrayList<>();
        for (String token : path.split("/")) {
            if (!token.isEmpty()) {
                tokens.add(token);
            }
        }
        return tokens;
    }

    private static String join(List<String> tokens) {
        return String.join("/", tokens);
    }

    FileSystemNode resolve(String path) {
        FileSystemNode current = path.startsWith("/") ? root : pwd;
        for (String token : tokenize(path)) {
            if (token.equals(".") || !(current instanceof DirectoryNode)) {
                continue;
            }
            if (token.equals("..")) {
                current = current.getParent() != null ? current.getParent() : root;
                continue;
            }
            FileSystemNode child = ((DirectoryNode) current).getChild(token);
            if (child == null) {
                return null;
            }
            current = child;
        }
        return current;
    }

    private Object[] resolveParentDirectory(String path) {
        List<String> tokens = tokenize(path);
        if (tokens.isEmpty()) {
            throw new FileSystemError("Invalid path: " + path);
        }
        String leafName = tokens.remove(tokens.size() - 1);

        FileSystemNode parent;
        if (tokens.isEmpty()) {
            parent = path.startsWith("/") ? root : pwd;
        } else {
            String parentPath = (path.startsWith("/") ? "/" : "") + join(tokens);
            parent = resolve(parentPath);
        }

        if (!(parent instanceof DirectoryNode)) {
            throw new FileSystemError("Parent directory does not exist for path: " + path);
        }
        return new Object[]{parent, leafName};
    }

    // Creates every missing directory along the path, like `mkdir -p`.
    DirectoryNode mkdir(String path) {
        DirectoryNode current = path.startsWith("/") ? root : pwd;
        for (String token : tokenize(path)) {
            FileSystemNode child = current.getChild(token);
            if (child == null) {
                DirectoryNode newDir = new DirectoryNode(token, current);
                current.addChild(newDir);
                current = newDir;
            } else if (child instanceof DirectoryNode) {
                current = (DirectoryNode) child;
            } else {
                throw new FileSystemError("'" + token + "' exists and is not a directory");
            }
        }
        return current;
    }

    FileNode touch(String path) {
        Object[] resolved = resolveParentDirectory(path);
        DirectoryNode parent = (DirectoryNode) resolved[0];
        String name = (String) resolved[1];
        if (parent.getChild(name) != null) {
            throw new FileSystemError("'" + path + "' already exists");
        }
        FileNode fileNode = new FileNode(name, parent);
        parent.addChild(fileNode);
        return fileNode;
    }

    void writeFile(String path, String data, boolean append) {
        FileSystemNode node = resolve(path);
        if (node == null) {
            node = touch(path);
        }
        if (!(node instanceof FileNode)) {
            throw new FileSystemError("'" + path + "' is not a file");
        }
        ((FileNode) node).write(data, append);
    }

    void writeFile(String path, String data) {
        writeFile(path, data, false);
    }

    String readFile(String path) {
        FileSystemNode node = resolve(path);
        if (!(node instanceof FileNode)) {
            throw new FileSystemError("'" + path + "' is not a file");
        }
        return ((FileNode) node).read();
    }

    void delete(String path) {
        Object[] resolved = resolveParentDirectory(path);
        DirectoryNode parent = (DirectoryNode) resolved[0];
        String name = (String) resolved[1];
        if (parent.removeChild(name) == null) {
            throw new FileSystemError("'" + path + "' does not exist");
        }
    }

    List<String> list(String path) {
        FileSystemNode node = resolve(path);
        if (!(node instanceof DirectoryNode)) {
            throw new FileSystemError("'" + path + "' is not a directory");
        }
        List<String> names = new ArrayList<>();
        for (FileSystemNode child : ((DirectoryNode) node).listChildren()) {
            names.add(child.getName());
        }
        Collections.sort(names);
        return names;
    }

    void cd(String path) {
        FileSystemNode node = resolve(path);
        if (!(node instanceof DirectoryNode)) {
            throw new FileSystemError("'" + path + "' is not a directory");
        }
        pwd = (DirectoryNode) node;
    }

    void move(String sourcePath, String destinationPath) {
        Object[] sourceResolved = resolveParentDirectory(sourcePath);
        DirectoryNode sourceParent = (DirectoryNode) sourceResolved[0];
        String sourceName = (String) sourceResolved[1];
        FileSystemNode node = sourceParent.getChild(sourceName);
        if (node == null) {
            throw new FileSystemError("'" + sourcePath + "' does not exist");
        }

        FileSystemNode destination = resolve(destinationPath);
        DirectoryNode targetParent;
        String targetName;
        if (destination instanceof DirectoryNode) {
            targetParent = (DirectoryNode) destination;
            targetName = node.getName();
        } else {
            Object[] destResolved = resolveParentDirectory(destinationPath);
            targetParent = (DirectoryNode) destResolved[0];
            targetName = (String) destResolved[1];
        }

        sourceParent.removeChild(sourceName);
        node.setName(targetName);
        node.setParent(targetParent);
        targetParent.addChild(node);
    }

    // Depth-first search for every node named `name`, returning full paths.
    List<String> search(String name) {
        List<String> matches = new ArrayList<>();
        walk(root, name, matches);
        return matches;
    }

    private void walk(FileSystemNode node, String name, List<String> matches) {
        if (node.getName().equals(name)) {
            matches.add(node.getPath());
        }
        if (node instanceof DirectoryNode) {
            for (FileSystemNode child : ((DirectoryNode) node).listChildren()) {
                walk(child, name, matches);
            }
        }
    }
}

public class FIleSystem_lld {
    public static void main(String[] args) {
        FileSystem fs = new FileSystem();

        fs.mkdir("/home/alice/docs");
        fs.writeFile("/home/alice/docs/todo.txt", "buy milk\n");
        fs.writeFile("/home/alice/docs/todo.txt", "walk the dog\n", true);
        fs.touch("/home/alice/notes.txt");
        fs.writeFile("/home/alice/notes.txt", "meeting at 5pm");

        System.out.println("Contents of /home/alice: " + fs.list("/home/alice"));
        System.out.println("todo.txt contents:");
        System.out.println(fs.readFile("/home/alice/docs/todo.txt"));

        System.out.println("Size of /home/alice: " + fs.resolve("/home/alice").getSize() + " bytes");

        fs.cd("/home/alice");
        System.out.println("pwd listing (relative path 'docs'): " + fs.list("docs"));

        fs.move("/home/alice/notes.txt", "/home/alice/docs/notes.txt");
        System.out.println("After move, /home/alice/docs: " + fs.list("/home/alice/docs"));

        System.out.println("Search for 'notes.txt': " + fs.search("notes.txt"));

        fs.delete("/home/alice/docs/todo.txt");
        System.out.println("After delete, /home/alice/docs: " + fs.list("/home/alice/docs"));
    }
}
