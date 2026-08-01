

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

import java.util.concurrent.locks.ReentrantLock;

abstract class FileSystemNode{
    String name; 
    DirectoryNode parent; 
    Instant createdAt;
    Instant modifiedAt; 
    ReentrantLock lock; 
    getName()
    setName();
    isDirectory(); 
    getSize(): Abstract
    getParent();
}

class FileNode extends FileSystemNode{
    StringBuilder content; 
    AtomicInteger size;  //thread safe size variable 
    + write(String data, boolean append)
    + read() 
    + getSize(){
        return size;
    } 
}

class DirectoryNode extends FileSystemNode{
    Map<String, FileSystemNode> childern; // ArrayList<FileSystemNode>
    addChild(FileSystemNode)    : void
    removeChild(String) : FileSystemNode;
    
    int getSize(){
        total = 0
        for(Each child in the childern){
            total += child.getSize();
        }
        return total;
    }
    listChildren() : List<FileSystemNode>
}

class FileSystem{
    DirectoryNode root; // path or reference to root 
    DirectoryNode pwd; 


    +touch(String path)
    +mkdir(String path)
    +writeFile(String path, String data, boolean append)
    +readFile(String path)
    +delete(String path)
    list(String path)
    move(String src, String dest)
    search(String name)
    resolve(String path) : FileSystemNode 
    resolveParentDirectory(String path)

}



FileSystemNode resolve(String path){
    String [] tokens = path.split("/");

    DirectoryNode current = root;
    for(int i = 0; i < tokens.length;i++){
        FileSystemNode next = current.getChild(tokens[i]);
        if(next != null){
            current = (DirectoryNode)next;
        }
    }
}






