#ifndef ABSTRACTFILE_H
#define ABSTRACTFILE_H

#include <string>

/**
 * @brief The AbstractFile class
 * Used in the TreeView model to represent root
 * with folders and files
 */

class AbstractFile
{

public:
    AbstractFile(const std::string& name, bool canRename, AbstractFile *parent = nullptr);
    virtual ~AbstractFile() = 0;
    std::string getName();
    bool hasParent();
    AbstractFile *getParent();
    void setParent(AbstractFile *parent);
    void setName(const std::string& name);
    void setCanRename(bool canRename);
    bool isRenamable();

protected:
    AbstractFile *parent;
    std::string name;
    bool canRename;
};

#endif // ABSTRACTFILE_H
