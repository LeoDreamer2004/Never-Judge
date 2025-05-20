#include "window.h"

#include <QLayout>
#include <QMessageBox>
#include <QSplitter>

#include "../util/file.h"
#include "setting.h"
#include "preview.h"
#include "aiAssistant.h"

IDEMainWindow::IDEMainWindow(QWidget *parent) : QMainWindow(parent) {
    ide = new IDE();
    leftNav = new LeftIconNavigateWidget(this);
    rightNav = new RightIconNavigateWidget(this);
    fileTree = new FileTreeWidget(this);
    terminal = new TerminalWidget(this);
    codeTab = new CodeTabWidget(this);
    menuBar = new MenuBarWidget(this);
    ojPreview = new OpenJudgePreviewWidget(this);
    aiAssistant = new AIAssistantWidget(codeTab, this);
    footer = &FooterWidget::instance();

    terminal->setVisible(false);
    ojPreview->setVisible(false);
    aiAssistant->setVisible(false);

    setup();
    connectSignals();
}

void IDEMainWindow::setup() {
    this->setWindowTitle(tr("Never Judge"));
    this->setMinimumSize(1366, 768);
    this->setMenuBar(menuBar);

    auto *mainLayout = new QHBoxLayout(this);
    auto *editLayout = new QVBoxLayout(this);
    mainLayout->addWidget(leftNav);
    mainLayout->addLayout(editLayout);
    mainLayout->addWidget(rightNav);

    auto *hSplitter = new QSplitter(Qt::Horizontal, this);
    hSplitter->addWidget(fileTree);
    hSplitter->addWidget(codeTab);
    hSplitter->addWidget(ojPreview);
    hSplitter->addWidget(aiAssistant);

    hSplitter->setStretchFactor(0, 2);
    hSplitter->setStretchFactor(1, 4);
    hSplitter->setStretchFactor(2, 2);
    hSplitter->setStretchFactor(3, 2);

    auto *vSplitter = new QSplitter(Qt::Vertical, this);
    vSplitter->addWidget(hSplitter);
    vSplitter->addWidget(terminal);
    vSplitter->setStretchFactor(0, 3);
    vSplitter->setStretchFactor(1, 1);
    editLayout->addWidget(vSplitter);

    auto *fullLayout = new QVBoxLayout(this);
    fullLayout->addLayout(mainLayout);
    fullLayout->addWidget(footer);
    fullLayout->setContentsMargins(0, 0, 0, 0);
    fullLayout->setSpacing(0);

    auto *centralWidget = new QWidget(this);
    centralWidget->setLayout(fullLayout);
    setCentralWidget(centralWidget);

    setStyleSheet(loadText("qss/window.css"));
}

void IDEMainWindow::connectSignals() {
    // Icon navigate
    connect(leftNav, &LeftIconNavigateWidget::toggleFileTree, fileTree,
            &FileTreeWidget::setVisible);
    connect(leftNav, &LeftIconNavigateWidget::toggleTerminal, terminal,
            &TerminalWidget::setVisible);
    connect(rightNav, &RightIconNavigateWidget::loginOJ, ojPreview,
            &OpenJudgePreviewWidget::loginOJ);
    connect(rightNav, &RightIconNavigateWidget::togglePreview, ojPreview,
            &OpenJudgePreviewWidget::setVisible);
    connect(rightNav, &RightIconNavigateWidget::openSetting, this, &IDEMainWindow::openSettings);
    // Connect AI Assistant toggle button
    connect(rightNav, &RightIconNavigateWidget::toggleAIAssistant, aiAssistant, &AIAssistantWidget::setVisible);

    // Connect OJ Preview Change to AI Assistant - 确保AI窗口会随着新下载的问题更新而更新
    connect(ojPreview, &OpenJudgePreviewWidget::currentIndexChanged, this, [this]() {
        // 当预览窗口的当前题目发生变化时，尝试更新AI窗口的问题信息
        if (aiAssistant->isVisible()) {
            aiAssistant->tryGetProblemInfo();
        }
    });

    // File system
    connect(menuBar, &MenuBarWidget::newFile, fileTree,
            [this] { fileTree->createNewFile(ide->curProject().getRoot()); });
    connect(menuBar, &MenuBarWidget::newFolder, fileTree,
            [this] { fileTree->createNewFolder(ide->curProject().getRoot()); });
    connect(menuBar, &MenuBarWidget::saveFile, codeTab, &CodeTabWidget::save);
    connect(menuBar, &MenuBarWidget::openFolder, this, &IDEMainWindow::openFolder);
    connect(fileTree, &FileTreeWidget::operateFile, codeTab, &CodeTabWidget::handleFileOperation);

    // Running
    connect(menuBar, &MenuBarWidget::runCode, this, &IDEMainWindow::runCurrentCode);

    // Edit
    connect(menuBar, &MenuBarWidget::openSettings, this, &IDEMainWindow::openSettings);

    // OJ
    connect(menuBar, &MenuBarWidget::downloadOJ, ojPreview,
            [this] { ojPreview->setVisible(true); });
    connect(menuBar, &MenuBarWidget::downloadOJ, ojPreview, &OpenJudgePreviewWidget::downloadOJ);
    connect(menuBar, &MenuBarWidget::batchDownloadOJ, ojPreview,
            [this] { ojPreview->setVisible(true); });
    connect(menuBar, &MenuBarWidget::batchDownloadOJ, ojPreview,
            &OpenJudgePreviewWidget::batchDownloadOJ);
    connect(menuBar, &MenuBarWidget::loginOJ, ojPreview, &OpenJudgePreviewWidget::loginOJ);
    connect(menuBar, &MenuBarWidget::submitOJ, this, &IDEMainWindow::submitCurrentCode);
    connect(ojPreview, &OpenJudgePreviewWidget::loginAs, menuBar, &MenuBarWidget::onLogin);
}

void IDEMainWindow::openFolder(const QString &folder) const {
    qDebug() << "IDEMainWindow: Opening folder" << folder;
    Project project(folder);
    ide->setProject(project);
    codeTab->setProject(&ide->curProject());
    fileTree->setRoot(project.getRoot());
    ojPreview->clear();
    terminal->setProject(&ide->curProject());
}

void IDEMainWindow::openSettings() {
    auto settings = new SettingsDialog(this);
    settings->exec();
}

void IDEMainWindow::runCurrentCode() const {
    // awake the terminal
    terminal->setVisible(true);

    LangFileInfo file = codeTab->currentFile();

    if (!file.isValid()) {
        QMessageBox::warning(menuBar, tr("错误"), tr("不存在的文件或非法文件"));
        return;
    }
    if (file.language() == Language::UNKNOWN) {
        QMessageBox::warning(menuBar, tr("错误"), tr("不支持运行的文件类型"));
        return;
    }

    codeTab->save(); // save the current file before running
    terminal->runCmd(Command::runFile(file));
}

void IDEMainWindow::submitCurrentCode() const {
    auto edit = codeTab->curEdit();
    if (!edit) {
        QMessageBox::warning(menuBar, tr("错误"), tr("你还没有打开一个项目！"));
        return;
    }
    // get the text on the current edit
    QString code = edit->toPlainText();
    if (code.isEmpty()) {
        QMessageBox::warning(menuBar, tr("错误"), tr("代码不能为空"));
        return;
    }
    ojPreview->submit(code);
}
