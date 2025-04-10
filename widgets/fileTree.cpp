#include "fileTree.h"

#include <QContextMenuEvent>
#include <QDesktopServices>
#include <QHeaderView>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QUrl>

#include "../ide/project.h"


FileTreeWidget::FileTreeWidget(QWidget *parent) : QTreeView(parent) {
    model = new QFileSystemModel(this);
    this->QTreeView::setModel(model);
    setup();
    connect(this, &QTreeView::clicked, this, &FileTreeWidget::clickFile);
    connect(this, &FileTreeWidget::rawOperateFile, this, &FileTreeWidget::handleRawFileOperation);
}

void FileTreeWidget::setRoot(const QString &root) {
    model->setRootPath(root);
    setRootIndex(model->index(root));
}

void FileTreeWidget::addFileOperationToMenu(QMenu &menu, const QString &file, const QString &label,
                                            FileOperation operation) {
    QAction *action = menu.addAction(label);
    for (const auto &shortcut: opShortcuts.keys()) {
        if (operation == opShortcuts.value(shortcut)) {
            action->setShortcut(shortcut);
            break;
        }
    }
    connect(action, &QAction::triggered, this, [this, file, operation] { emit rawOperateFile(file, operation); });
}

void FileTreeWidget::setup() {
    // TODO: Finish the title
    header()->hide();
    // only keep filenames
    for (int i = 1; i < model->columnCount(); ++i) {
        this->hideColumn(i);
    }
}

void FileTreeWidget::contextMenuEvent(QContextMenuEvent *event) {
    QModelIndex index = indexAt(event->pos());
    if (!index.isValid()) {
        return;
    }

    QString filePath = model->filePath(index);
    QMenu menu(this);

    addFileOperationToMenu(menu, filePath, tr("打开"), OPEN);
    addFileOperationToMenu(menu, filePath, tr("在本地打开"), OPEN_LOCALLY);
    addFileOperationToMenu(menu, filePath, tr("重命名"), RENAME);
    addFileOperationToMenu(menu, filePath, tr("删除"), DELETE);

    menu.addSeparator();

    if (model->isDir(index)) {
        QAction *newFileAction = menu.addAction(tr("新建文件"));
        connect(newFileAction, &QAction::triggered, this, [this, filePath] { createNewFile(filePath); });

        QAction *newFolderAction = menu.addAction(tr("新建文件夹"));
        connect(newFolderAction, &QAction::triggered, this, [this, filePath] { createNewFolder(filePath); });
    }

    menu.exec(event->globalPos());
}

QMap<Qt::Key, FileOperation> FileTreeWidget::opShortcuts = {
        {Qt::Key_Enter, OPEN},
        {Qt::Key_F1, OPEN_LOCALLY},
        {Qt::Key_F2, RENAME},
        {Qt::Key_Delete, DELETE},
};

void FileTreeWidget::keyPressEvent(QKeyEvent *event) {
    auto key = static_cast<Qt::Key>(event->key());
    if (opShortcuts.contains(key)) {
        QModelIndex index = currentIndex();
        if (!index.isValid()) {
            return;
        }
        QString filePath = model->filePath(index);
        emit rawOperateFile(filePath, opShortcuts.value(key));
    } else {
        QTreeView::keyPressEvent(event);
    }
}

void FileTreeWidget::clickFile(const QModelIndex &index) {
    if (model->isDir(index)) {
        return;
    }
    QString filePath = model->filePath(index);
    emit rawOperateFile(filePath, OPEN);
}

void FileTreeWidget::createNewFile(const QString &dir) {
    bool ok;
    QString fileName = QInputDialog::getText(this, tr("新建文件"), tr("请输入文件名:"), QLineEdit::Normal, "", &ok);

    if (ok && !fileName.isEmpty()) {
        QString newFilePath = dir + (dir.endsWith('/') ? "" : "/") + fileName;

        if (QFile::exists(newFilePath)) {
            QMessageBox::warning(this, tr("错误"), tr("文件已存在: %1").arg(newFilePath));
            return;
        }

        QFile file(newFilePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.close();
        } else {
            QMessageBox::warning(this, tr("错误"), tr("无法创建文件: %1").arg(newFilePath));
        }
    }
}

void FileTreeWidget::createNewFolder(const QString &dir) {
    bool ok;
    QString folderName =
            QInputDialog::getText(this, tr("新建文件夹"), tr("请输入文件夹名:"), QLineEdit::Normal, "", &ok);

    if (ok && !folderName.isEmpty()) {
        QString newFolderPath = dir + (dir.endsWith('/') ? "" : "/") + folderName;

        if (QDir(newFolderPath).exists()) {
            QMessageBox::warning(this, tr("错误"), tr("文件夹已存在: %1").arg(newFolderPath));
            return;
        }

        QDir qDir;
        if (!qDir.mkdir(newFolderPath)) {
            QMessageBox::warning(this, tr("错误"), tr("无法创建文件夹: %1").arg(newFolderPath));
        }
    }
}

void FileTreeWidget::handleRawFileOperation(const QString &filename, const FileOperation operation) {
    FileInfo file(filename);
    bool ok = true;

    if (!file.exists()) {
        QMessageBox::warning(this, tr("错误"), tr("文件不存在: %1").arg(filename));
        return;
    }

    switch (operation) {
        case OPEN: {
            // Open in the code editor, so do nothing here
            break;
        }

        case OPEN_LOCALLY: {
            QString folderPath = file.isDir() ? filename : file.path();
            ok = QDesktopServices::openUrl(QUrl::fromLocalFile(folderPath));
            if (!ok) {
                QMessageBox::warning(this, tr("错误"), tr("无法打开目录: %1").arg(folderPath));
            }
            break;
        }

        case RENAME: {
            QString newName = QInputDialog::getText(this, tr("重命名"), tr("请输入新文件名:"), QLineEdit::Normal,
                                                    file.fileName(), &ok);
            if (ok && !newName.isEmpty()) {
                QString newPath = file.path() + "/" + newName;
                ok = QFile::rename(filename, newPath);
                if (!ok) {
                    QMessageBox::warning(this, tr("错误"), tr("重命名失败: %1").arg(filename));
                }
            }
            break;
        }

        case DELETE: {
            QString fileType = file.isDir() ? tr("目录") : tr("文件");

            if (QMessageBox::question(this, tr("确认删除"), tr("确定要删除%1吗？\n%2").arg(fileType, filename),
                                      QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
                break;
            }
            ok = file.isDir() ? QDir(filename).removeRecursively() : QFile::remove(filename);
            if (!ok) {
                QMessageBox::warning(this, tr("错误"), tr("无法删除%1: %2").arg(fileType, filename));
            }
            break;
        }

        default:
            QMessageBox::warning(this, tr("错误"), tr("未知的操作类型"));
            break;
    }

    if (ok) {
        emit operateFile(filename, operation);
    }
}
