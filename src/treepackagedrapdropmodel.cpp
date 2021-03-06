#include "treepackagedragdropmodel.h"
#include "folder.h"
#include "realfile.h"
#include "filesignatureinfo.hpp"
#include <QIcon>
#include <QFileInfo>
#include <QMimeData>
#include <QUrl>

TreePackageDragDropModel::TreePackageDragDropModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    // default tree of a debian package
    tree = new Folder("packagename");
    tree->add(new Folder("DEBIAN", false)).add(new RealFile("control", false));
    tree->add(new Folder("usr", false)).add(new Folder("bin", false));
}

TreePackageDragDropModel::~TreePackageDragDropModel()
{
    delete tree;
    // all the RealFile* contained in the list are already deleted by the "delete tree;"
    fileFromUser.clear();
    fileFromProgram.clear();
}

void TreePackageDragDropModel::resetToDefault()
{
    beginResetModel();
    delete tree;
    fileFromUser.clear();
    tree = new Folder("packagename");
    tree->add(new Folder("DEBIAN", false)).add(new RealFile("control", false));
    tree->add(new Folder("usr", false)).add(new Folder("bin", false));
    endResetModel();
}

QModelIndex TreePackageDragDropModel::index(int row, int column, const QModelIndex &parent) const
{
    QModelIndex ret;
    if (hasIndex(row, column, parent)){
        AbstractFile *parentItem;
        if (parent.isValid()){
            parentItem = static_cast<AbstractFile*>(parent.internalPointer());
        } else {
            parentItem = tree; // root
        }
        if(Folder* f = dynamic_cast<Folder*>(parentItem)) {
           ret = createIndex(row, column, f->child(row));
        }
    }
    return ret;
}

QModelIndex TreePackageDragDropModel::parent(const QModelIndex &index) const
{
    QModelIndex ret;
    if (index.isValid()){
        AbstractFile *child = static_cast<AbstractFile*>(index.internalPointer());
        AbstractFile *parent = child->getParent();
        if (parent && parent != child){
            ret = createIndex(1, 0, parent);
        }
    }
    return ret;
}

QVariant TreePackageDragDropModel::data(const QModelIndex &index, int role) const
{
    QVariant ret;
    if (index.isValid()){
        switch (role) {
        case Qt::DisplayRole:
            ret = displayRole(index);
            break;
        case Qt::DecorationRole:
            ret = decorationRole(index);
            break;
        case Qt::EditRole:
            ret = displayRole(index);
            break;
        case Qt::ToolTipRole:
            ret = toolTipRole(index);
            break;
        default:
            break;
        }
    }
    return ret;
}

QVariant TreePackageDragDropModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(section);
    QVariant ret;
    if (orientation == Qt::Horizontal){
        switch (role) {
        case Qt::DisplayRole:
            ret = QString("Package name: %1").arg(tree->getName().c_str());
            break;
        default:
            break;
        }
    }
    return ret;
}

bool TreePackageDragDropModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    bool ret = false;
    if (index.isValid() && role == Qt::EditRole){
        AbstractFile *af = static_cast<AbstractFile*>(index.internalPointer());
        af->setName(value.toString().toStdString());
        ret = true;
    }
    return ret;
}

bool TreePackageDragDropModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(action);
    Q_UNUSED(row);
    Q_UNUSED(column);
    bool ret = false;
    if (data->hasUrls()){
        // from filesystem
        ret = QFileInfo(data->urls().first().path()).isFile();
    } else {
        // from tree
        if (parent.isValid()){
            if (Folder *parent_f = static_cast<Folder*>(parent.internalPointer())){
                if (parent_f != tree && parent_f->getName() != "DEBIAN")
                    ret = true;
            }
        }
    }
    return ret;
}

bool TreePackageDragDropModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(action);
    Q_UNUSED(row);
    Q_UNUSED(column);
    bool ret = false;
    if (data->hasUrls()){
        // add from filesystem
        FileSignatureInfo *fsi = new FileSignatureInfo(data->urls().first().path().toStdString());
        int before = tree->count(true);
        addFileInfo(fsi);
        if (before < tree->count(true)){
            ret = true;
        } else {
            delete fsi;
        }
    } else {
        // move from tree
        if (parent.isValid() && data->formats().first() == "debpac/realfile"){
            const QString realfile_ptr = data->text();
            qintptr rf_ptr = realfile_ptr.toLong(nullptr, 16);
            RealFile *rf = reinterpret_cast<RealFile*>(rf_ptr);
            if (rf){
                QModelIndex old = indexByAbstractFile(rf);
                if (old.isValid()){
                    beginRemoveRows(old.parent(), old.row(), old.row());
                    tree->getChild<Folder*>("usr")->remove(rf, true);
                    endRemoveRows();
                    Folder *new_emplacement = static_cast<Folder*>(parent.internalPointer());
                    if (new_emplacement){
                        beginInsertRows(parent, new_emplacement->count(false), new_emplacement->count(false));
                        new_emplacement->add(rf);
                        endInsertRows();
                    }
                }
            }
        }
    }
    return ret;
}

QMimeData *TreePackageDragDropModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = QAbstractItemModel::mimeData(indexes);
    if (indexes.first().isValid()){
        RealFile *rf = static_cast<RealFile*>(indexes.first().internalPointer());
        if (rf){
            mimeData->setText(QString("0x%1").arg((qintptr) rf, QT_POINTER_SIZE, 16, QChar('0')));
        }
    }
    return mimeData;
}

QStringList TreePackageDragDropModel::mimeTypes() const
{
    QStringList mime_types = QAbstractItemModel::mimeTypes();
    mime_types.prepend("debpac/realfile");
    return mime_types;
}

Qt::ItemFlags TreePackageDragDropModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags ret = QAbstractItemModel::flags(index);
    if (index.isValid()){
        AbstractFile *af = static_cast<AbstractFile*>(index.internalPointer());
        if (RealFile *rf = dynamic_cast<RealFile*>(af)){
            if (rf->getParent()->getName() != "DEBIAN"){
                ret |= Qt::ItemIsDragEnabled;
            }
        }
        if (Folder *f = dynamic_cast<Folder*>(af)){
            if (f->getName() != "DEBIAN")
                ret |= Qt::ItemIsDropEnabled;
        }
        if (af->isRenamable()){
            ret |= Qt::ItemIsEditable;
        }
    } else {
        ret |= Qt::ItemIsDropEnabled;
    }
    return ret;
}

int TreePackageDragDropModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    int ret = 0;
    AbstractFile *parentItem = static_cast<AbstractFile*>(parent.internalPointer());
    if (parent.isValid()){
        if(Folder* f = dynamic_cast<Folder*>(parentItem)) {
            ret = f->count(false);
        } else {
            ret = 0;
        }
    } else {
        // root
        ret = tree->count(false);
    }
    return ret;
}

int TreePackageDragDropModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QModelIndex TreePackageDragDropModel::indexByAbstractFile(AbstractFile *internal)
{
    Folder *parent = dynamic_cast<Folder*>(internal->getParent());
    QModelIndex ret = createIndex(parent->child(internal), 0, internal);
    return ret;
}

void TreePackageDragDropModel::addFileInfo(FileSignatureInfo *fsi)
{
    QString folder;
    switch (fsi->getCategory()) {
    case FileSignatureInfo::BINARY:
        folder = "usr/bin";
        break;
    case FileSignatureInfo::AUDIO:
        folder = "usr/share/"+QString(tree->getName().c_str())+"/sounds";
        break;
    case FileSignatureInfo::IMAGE:
        if (QPixmap(fsi->getPath().c_str()).width() == QPixmap(fsi->getPath().c_str()).height()){
            folder = "usr/share/icons/hicolor/"+QString("%1x%1").arg(QPixmap(fsi->getPath().c_str()).width())+"/apps";
        } else {
            folder = "usr/share/"+QString(tree->getName().c_str())+"/images";
        }
        break;
    case FileSignatureInfo::PACKAGE:
    case FileSignatureInfo::ARCHIVE:
    default:
        folder = "usr/share/"+QString(tree->getName().c_str());
        break;
    }
    if (!folder.isEmpty()){
        addFileInfo(folder, fsi);
    }
}

void TreePackageDragDropModel::addFileInfo(const QString &path, FileSignatureInfo *fsi)
{
    QModelIndex parentIndex = index(0);
    int renameFolderIndex = -1;
    switch (fsi->getCategory()) {
    case FileSignatureInfo::PACKAGE:
    case FileSignatureInfo::ARCHIVE:
    case FileSignatureInfo::AUDIO:
    case FileSignatureInfo::UNKNOW:
        renameFolderIndex = 2;
        break;
    case FileSignatureInfo::IMAGE:
        if (!(QPixmap(fsi->getPath().c_str()).width() == QPixmap(fsi->getPath().c_str()).height())){
            renameFolderIndex = 2;
        }
        break;
    default:
        break;
    }
    int idx = 0;
    QStringList sl = path.split("/");
    Folder *f = tree;
    for (QString s : sl){
        if (Folder *current = f->getChild<Folder*>(s.toStdString())){
            f = current;
        } else {
            int at = f->count(false);
            beginInsertRows(parentIndex, at, at);
            Folder *nf = new Folder(s.toStdString(), idx==renameFolderIndex);
            f->add(nf);
            f = nf;
            endInsertRows();
        }
        parentIndex = indexByAbstractFile(f);
        idx++;
    }
    if (f){
        int at = f->count(false);
        beginInsertRows(parentIndex, at, at);
        RealFile *rf = new RealFile(QFileInfo(fsi->getPath().c_str()).completeBaseName().toStdString().c_str(), false, fsi);
        f->add(rf);
        fileFromUser.append(rf);
        endInsertRows();
    }
}

QVector<RealFile *> TreePackageDragDropModel::getFileFromUser()
{
    return fileFromUser;
}

QVector<RealFile *> TreePackageDragDropModel::getFileFromProgram()
{
    return fileFromProgram;
}

Folder *TreePackageDragDropModel::getRoot()
{
    return tree;
}

void TreePackageDragDropModel::addScriptFile(const QString &name)
{
    Folder *debian = tree->getChild<Folder*>("DEBIAN");
    if (debian){
        if (!debian->containFile(name.toStdString())){
            int at = debian->count(false);
            beginInsertRows(index(1), at, at);
            RealFile *rf = new RealFile(name.toStdString(), false);
            debian->add(rf);
            fileFromProgram.append(rf);
            endInsertRows();
        }
    }
}

void TreePackageDragDropModel::removeScriptFile(const QString &name)
{
    RealFile *removedFile = Q_NULLPTR;
    for (int i=0; i<fileFromProgram.size() && !removedFile; i++){
        if (fileFromProgram.at(i)->getName() == name.toStdString()){
            removedFile = fileFromProgram.at(i);
        }
    }
    QModelIndex index = indexByAbstractFile(removedFile);
    if (index.isValid()){
        beginRemoveRows(index.parent(), index.row(), index.row());
        Folder *parent = static_cast<Folder*>(index.parent().internalPointer());
        if (parent){
            if (parent->remove(removedFile, false)){
                fileFromProgram.remove(fileFromProgram.indexOf(removedFile));
                delete removedFile;
            }
        }
        endRemoveRows();
    }
}

void TreePackageDragDropModel::addDesktopFile(const QString &name)
{
    Q_UNUSED(name);
    QModelIndex parentIndex = index(0);
    QString folder = "usr/share/applications";
    QStringList sl = folder.split("/");
    Folder *f = tree;
    for (QString s : sl){
        if (Folder *current = f->getChild<Folder*>(s.toStdString())){
            f = current;
        } else {
            int at = f->count(false);
            beginInsertRows(parentIndex, at, at);
            Folder *nf = new Folder(s.toStdString(), false);
            f->add(nf);
            f = nf;
            endInsertRows();
        }
        parentIndex = index(0, 0, parentIndex);
    }
    if (f && !f->containFile(tree->getName()+".desktop")){
        int at = f->count(false);
        beginInsertRows(parentIndex, at, at);
        RealFile *rf = new RealFile(tree->getName()+".desktop", true);
        f->add(rf);
        fileFromProgram.append(rf);
        endInsertRows();
    }
}

void TreePackageDragDropModel::changePackageName(const QString &pname)
{
    if (Folder *apps_folder = tree->containFolder("applications", true)){
        if (RealFile *desktop = apps_folder->containFile(tree->getName()+".desktop")){
            desktop->setName(pname.toStdString()+".desktop");
            emit changeDesktopTab(tree->getName().c_str(), pname);
        }
    }
    tree->renameFolder(tree->getName(), pname.toStdString(), true);
    emit headerDataChanged(Qt::Horizontal, 0, 0);
}

void TreePackageDragDropModel::createFolder(const QModelIndex &index)
{
    if (index.isValid()){
        AbstractFile *af = static_cast<AbstractFile*>(index.internalPointer());
        // only if it's folder
        if (Folder *f = dynamic_cast<Folder*>(af)){
            beginInsertRows(index, f->count(false), f->count(false));
            f->add(new Folder("new_folder", true));
            endInsertRows();
        }
    }
}

void TreePackageDragDropModel::removeFolder(const QModelIndex &index)
{
    if (index.isValid()){
        AbstractFile *af = static_cast<AbstractFile*>(index.internalPointer());
        // only if it's folder
        if (Folder *f = dynamic_cast<Folder*>(af)){
            Folder *fparent = static_cast<Folder*>(index.parent().internalPointer());
            beginRemoveRows(index.parent(), index.row(), index.row());
            if (fparent->remove(f, false))
                delete f;
            endRemoveRows();
        }
    }
}

QVariant TreePackageDragDropModel::displayRole(const QModelIndex &index) const
{
    QString ret = QString(static_cast<AbstractFile*>(index.internalPointer())->getName().c_str());
    return ret;
}

QVariant TreePackageDragDropModel::decorationRole(const QModelIndex &index) const
{
    QIcon ret;
    AbstractFile *af = static_cast<AbstractFile*>(index.internalPointer());
    if(dynamic_cast<Folder*>(af)) {
        ret = QIcon("://icon/folder.png");
    } else {
        ret = dynamic_cast<RealFile*>(af)->getFileSignatureInfo().getIcon();
    }
    return ret;
}

QVariant TreePackageDragDropModel::toolTipRole(const QModelIndex &index) const
{
    QString ret;
    AbstractFile *af = static_cast<AbstractFile*>(index.internalPointer());
    if (RealFile *rf = dynamic_cast<RealFile*>(af)){
        FileSignatureInfo fi = rf->getFileSignatureInfo();
        ret = "<b>[File]</b> " + QString(af->getName().c_str()) + "<br>";
        if (fi.getCategory() == FileSignatureInfo::INEXISTANT){
            ret += "From: <i>debpac</i>";
        } else {
            ret += "From: <i>File system</i><br>";
            ret += "Magic number: <i>" + QString("%1 (%2)").arg(fi.getHex_signature().c_str(), fi.getExtension().c_str()) + "</i>";
        }
    } else {
        ret = "<b>[Folder]</b> "+QString(af->getName().c_str());
    }
    return ret;
}
