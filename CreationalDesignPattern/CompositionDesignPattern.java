package CreationalDesignPattern;
import java.util.ArrayList;
import java.util.List;

interface IFileSystemNode{
    int getSize(); // 
    String getName();
    void tree();
}


class File implements IFileSystemNode{
    String name; 
    String content;
    public File(String name, String content){
        this.name = name;
        this.content = content;
    }
    @Override
    public int getSize() {
        return content.length();
    }

    @Override
    public String getName() {
        return name;
    }

    @Override
    public void tree() {
        System.out.println("File "+name);
    }

}

class Folder implements IFileSystemNode{
    String name;
    List<IFileSystemNode> childrens;

    public Folder(String name){
        this.name = name;
        childrens = new ArrayList<>();
    }
    public void touch(String name, String content){
        childrens.add(new File(name, content));
    }

    public void mkdir(String name){
        childrens.add(new Folder(name));
    }

    @Override
    public int getSize() {
        int size = 0;
        for(IFileSystemNode node : childrens){
            size += node.getSize();
        }
        return size;
    }

    @Override
    public String getName() {
        return name;
    }

    @Override
    public void tree() {
        System.out.println("Folder:-> "+name);
        for(IFileSystemNode node : childrens){
            node.tree();
        }
    }

}

class FileSystemManager{
    String pathToCd;


    void goToCurrentDirectory(){

    }
}

public class CompositionDesignPattern{
    public static void main(String[] args) {
        // Root
            // System.txt
        // Doc
            // pradeep_lld.txt 
            // DSA
                // pradeep_dsa.txt 
        
        
        
        Folder root = new Folder("root");
        root.touch("System.txt", "dasdsadasd");
        root.mkdir("Doc");
        

        // File system_txt = new File("system.txt","system file");
        // root.AddFileSystemNode(system_txt);

        // Folder doc = new Folder("doc");
        // File pradeep_lld = new File("pradeep_lld.txt","adsdadasdas file");
        // doc.AddFileSystemNode(pradeep_lld);


        // Folder dsa = new Folder("dsa");
        // File pradeep_dsa = new File("pradeep_dsa.txt","asdasdasda file");
        // dsa.AddFileSystemNode(pradeep_dsa);
        
        // root.AddFileSystemNode(doc);
        // root.AddFileSystemNode(dsa);




     }
}






