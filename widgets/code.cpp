#include <QFile>
#include <QMessageBox>
#include <QPainter>
#include <QVBoxLayout>

#include "../ide/highlighter.h"
#include "../ide/lsp.h"
#include "../util/file.h"
#include "code.h"
#include "footer.h"
#include "icon.h"

/* Completion list */

CompletionList::CompletionList(CodeEditWidget *codeEdit) :
    QListWidget(codeEdit), codeEdit(codeEdit) {
    setWindowFlags(Qt::Popup);
    setSelectionMode(SingleSelection);
    setFocusPolicy(Qt::StrongFocus);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    hide();

    connect(this, &QListWidget::itemClicked, this, &CompletionList::onItemClicked);
}

void CompletionList::updateHeight() {
    const int itemHeight = sizeHintForRow(0);
    // at maximum 8 items
    const int visibleItems = qMin(count(), 8);

    int totalHeight = visibleItems * itemHeight + 2 * frameWidth();

    setFixedHeight(totalHeight);
    setFixedWidth(400);
}

void CompletionList::onItemClicked(const QListWidgetItem *item) {
    emit completionSelected(item->data(Qt::UserRole).toString());
}

void CompletionList::keyPressEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_Up || e->key() == Qt::Key_Down) {
        QListWidget::keyPressEvent(e);
    } else if (e->key() == Qt::Key_Tab) {
        if (currentItem()) {
            emit completionSelected(currentItem()->data(Qt::UserRole).toString());
            hide();
        }
    } else if (e->key() == Qt::Key_Escape) {
        hide();
    } else {
        codeEdit->keyPressEvent(e);
    }
}

void CompletionList::readCompletions(const CompletionResponse &response) {
    codeEdit->requireCompletion = response.incomplete;
    completions = response.items;
}

void CompletionList::update(const QString &curWord) {
    clear();
    for (const auto &item: completions) {
        if (item.insertText.startsWith(curWord)) {
            addCompletionItem(item);
        }
    }
}

void CompletionList::addCompletionItem(const CompletionItem &item) {
    auto *listItem = new QListWidgetItem(this);

    auto *widget = new QWidget(this);

    auto *layout = new QHBoxLayout(widget);
    layout->setContentsMargins(5, 2, 5, 2);

    auto *textLabel = new QLabel(item.label, this);
    textLabel->setMaximumWidth(300);
    textLabel->setFont(font());
    layout->addWidget(textLabel);
    layout->addStretch();

    static const QMap<CompletionItem::ItemKind, QString> kindIconMap = {
            {CompletionItem::Class, "code/class"},
            {CompletionItem::Function, "code/function"},
            {CompletionItem::Variable, "code/variable"},
            {CompletionItem::Module, "code/module"},
            {CompletionItem::Function, "code/function"},
            {CompletionItem::Keyword, "code/keyword"}};

    auto *iconBtn = new IconPushButton(this);
    auto iconPath = kindIconMap.value(item.kind, "code/text");
    iconBtn->setIconFromResName(iconPath);
    iconBtn->setDisabled(true);
    iconBtn->setStyleSheet(loadText("qss/completion.css"));
    layout->addWidget(iconBtn);

    widget->setLayout(layout);

    listItem->setSizeHint(widget->sizeHint());
    setItemWidget(listItem, widget);

    listItem->setData(Qt::UserRole, item.insertText);
}

void CompletionList::display() {
    setCurrentRow(0);
    show();
    setFocus();
    updateHeight();
}

/* Line number area */

LineNumberArea::LineNumberArea(CodeEditWidget *codeEdit) : QWidget(codeEdit), codeEdit(codeEdit) {}

int LineNumberArea::getWidth() const {
    int digits = 1;
    int max = qMax(1, codeEdit->blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    // at least 3 digits width
    int fontWidth = codeEdit->fontMetrics().horizontalAdvance(QLatin1Char('9')) * qMax(digits, 3);
    int marginWidth = L_MARGIN + R_MARGIN;
    return fontWidth + marginWidth;
}

QSize LineNumberArea::sizeHint() const { return {getWidth(), 0}; }

void LineNumberArea::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.fillRect(event->rect(), QColor(0x252526));

    QTextBlock block = codeEdit->firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = static_cast<int>(
            codeEdit->blockBoundingGeometry(block).translated(codeEdit->contentOffset()).top());
    int bottom = top + static_cast<int>(codeEdit->blockBoundingRect(block).height());

    painter.setFont(codeEdit->font());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);

            if (blockNumber == codeEdit->textCursor().blockNumber()) {
                painter.setPen(QColor(0xFFFFFF));
            } else {
                painter.setPen(QColor(0x858585));
            }

            painter.drawText(0, top, this->width() - R_MARGIN, fontMetrics().height(),
                             Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + static_cast<int>(codeEdit->blockBoundingRect(block).height());
        ++blockNumber;
    }
};

const int LineNumberArea::L_MARGIN = 5;
const int LineNumberArea::R_MARGIN = 5;

/* Welcome widget */

WelcomeWidget::WelcomeWidget(QWidget *parent) : QWidget(parent) { setup(); }

void WelcomeWidget::setup() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(30);

    auto *logoLabel = new QLabel(this);
    logoLabel->setText(loadText("logo.txt"));

    QFont logoFont("Consolas", 9);
    logoLabel->setFont(logoFont);
    logoLabel->setStyleSheet(R"(
        color: #569CD6;
        background-color: transparent;
        margin: 0;
        padding: 0;
    )");

    auto *shortcutLabel = new QLabel(this);
    shortcutLabel->setText("<p style='font-size: 16px; color: #D4D4D4;'>Ctrl+N 新建文件</p>"
                           "<p style='font-size: 16px; color: #D4D4D4;'>Ctrl+O 打开项目</p>"
                           "<p style='font-size: 16px; color: #D4D4D4;'>Ctrl+S 保存文件</p>"
                           "<p style='font-size: 16px; color: #D4D4D4;'>Ctrl+R 运行代码</p>");
    shortcutLabel->setAlignment(Qt::AlignCenter);

    mainLayout->addStretch();
    mainLayout->addWidget(logoLabel);
    mainLayout->addWidget(shortcutLabel);
    mainLayout->addStretch();

    setLayout(mainLayout);
}


/* Code plain text edit widget */

CodeEditWidget::CodeEditWidget(const QString &filename, QWidget *parent) :
    QPlainTextEdit(parent), server(nullptr), modified(false), requireCompletion(true) {
    lna = new LineNumberArea(this);
    cl = new CompletionList(this);
    file = LangFileInfo(filename);
    highlighter = HighlighterFactory::getHighlighter(file.language(), document());

    readFile();
    setup();
    adaptViewport();

    connect(this, &CodeEditWidget::blockCountChanged, this, &CodeEditWidget::adaptViewport);
    connect(this, &CodeEditWidget::updateRequest, this, &CodeEditWidget::updateLineNumberArea);
    connect(this, &CodeEditWidget::cursorPositionChanged, this, &CodeEditWidget::highlightLine);
    connect(this, &CodeEditWidget::cursorPositionChanged, cl, &QWidget::hide);
    connect(this, &QPlainTextEdit::textChanged, this, &CodeEditWidget::onTextChanged);
    connect(cl, &CompletionList::completionSelected, this, &CodeEditWidget::insertCompletion);
    runServer();
}

QCoro::Task<> CodeEditWidget::runServer() {
    server = co_await LanguageServers::get(file.language());
    if (server == nullptr) {
        co_return;
    }
    auto response = co_await server->initialize(file.path(), {});
    if (!response.ok) {
        qWarning() << "Server of language" << langName(file.language()) << "initialized failed";
    }
    co_return;
}

void CodeEditWidget::setup() {
    Configs::bindHotUpdateOn(this, "codeFont", &CodeEditWidget::onSetFont);
    Configs::instance().manuallyUpdate("codeFont");
}

void CodeEditWidget::onSetFont(const QJsonValue &fontJson) {
    QJsonObject obj = fontJson.toObject();
    QFont font;
    font.setFamily(obj["family"].toString());
    font.setPointSize(obj["size"].toInt());
    setTabStopDistance(3 * font.pointSize());
    setFont(font);
    lna->setFont(font);

    QFont sFont(font);
    sFont.setPointSize(font.pointSize() - 3); // smaller than edit
    cl->setFont(sFont);
}

void CodeEditWidget::resizeEvent(QResizeEvent *event) {
    QPlainTextEdit::resizeEvent(event);

    auto cr = contentsRect();
    lna->setGeometry(QRect(cr.left(), cr.top(), lna->getWidth(), cr.height()));
}


void CodeEditWidget::keyPressEvent(QKeyEvent *e) {
    QPlainTextEdit::keyPressEvent(e);
    updateCursorPosition();
}

void CodeEditWidget::updateCursorPosition() const {
    if (auto *highlighter = document()->findChild<QSyntaxHighlighter *>()) {
        if (auto *hl = dynamic_cast<Highlighter *>(highlighter)) {
            hl->setCursorPosition(textCursor().position());
        }
    }
}

void CodeEditWidget::adaptViewport() { setViewportMargins(lna->getWidth(), 0, 0, 0); }

QCoro::Task<> CodeEditWidget::askForCompletion() const {
    if (!server) {
        co_return;
    }
    auto cursor = textCursor();
    cursor.select(QTextCursor::WordUnderCursor);
    auto word = cursor.selectedText();
    if (word.isEmpty()) {
        co_return;
    }
    co_await server->didOpen({file.filePath(), toPlainText()});

    auto completion = co_await server->completion({file.filePath()},
                                                  {cursor.blockNumber(), cursor.columnNumber()});
    for (const auto &item: completion.items) {
        if (item.insertText == word) {
            co_return; // The word is finished and do not give completions
        }
    }

    cl->readCompletions(completion);
    auto rect = cursorRect();
    auto pos = mapToGlobal(QPoint(rect.right(), rect.bottom()));
    cl->move(pos);

    co_return;
}

void CodeEditWidget::updateCompletionList() {
    auto cursor = textCursor();
    cursor.select(QTextCursor::WordUnderCursor);
    auto word = cursor.selectedText();
    if (word.isEmpty()) {
        requireCompletion = true;
        return;
    }
    cl->update(word);
    if (cl->count() != 0) {
        cl->display();
    } else {
        cl->hide();
    }
}

void CodeEditWidget::insertCompletion(const QString &completion) {
    auto cursor = textCursor();
    cursor.select(QTextCursor::WordUnderCursor);
    cursor.removeSelectedText();
    cursor.insertText(completion);
    cl->hide();
    setFocus();
    requireCompletion = true;
}

void CodeEditWidget::updateLineNumberArea(const QRect &rect, int dy) {
    if (dy) {
        lna->scroll(0, dy);
    } else {
        lna->update(0, rect.y(), lna->width(), rect.height());
    }

    if (rect.contains(viewport()->rect())) {
        adaptViewport();
    }
}

const LangFileInfo &CodeEditWidget::getFile() const { return file; }

QString CodeEditWidget::getTabText() const { return file.fileName(); };

void CodeEditWidget::highlightLine() {
    QList<QTextEdit::ExtraSelection> selections;
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = QColor(0x222222).lighter(160);
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        selections.append(selection);
    }
    setExtraSelections(selections);
}

#define MAX_BUFFER_SIZE (1024 * 1024)

void CodeEditWidget::readFile() {
    QFile check(file.filePath());
    if (!check.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "CodeEditWidget::readFile: Failed to open file " << file.filePath();
        return;
    }

    // Check if the file is binary
    QTextStream in(&check);
    in.setAutoDetectUnicode(true);
    int lineCnt = 0;
    while (!in.atEnd()) {
        auto line = in.readLine();
        if (++lineCnt > 50) {
            break;
        }
        for (const auto &c: line) {
            if (!c.isPrint() && !c.isSpace() && !c.isPunct()) {
                if (c.category() != QChar::Mark_NonSpacing && // Mn
                    c.category() != QChar::Symbol_Other && // So
                    c.category() != QChar::Other_Surrogate && // Cs
                    c.category() != QChar::Other_Format) {
                    setReadOnly(true);
                    lna->setVisible(false);
                    setPlainText(tr("文件格式不支持"));
                    check.close();
                    return;
                }
            }
        }
    }

    QFile read(file.filePath());
    if (!read.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "CodeEditWidget::readFile: Failed to open file " << file.filePath();
        return;
    }
    QString buffer;
    if (read.size() > MAX_BUFFER_SIZE) {
        buffer = tr("文件过大，无法在编辑器内打开");
        lna->setVisible(false);
        setReadOnly(true);
    } else {
        buffer = read.readAll();
    }
    setPlainText(buffer);
    check.close();
}

void CodeEditWidget::saveFile() {
    QFile qfile(file.filePath());
    if (!qfile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "CodeEditWidget::saveFile: Failed to open file " << file.filePath();
        return;
    }
    modified = false;
    qfile.write(toPlainText().toUtf8());
    qfile.close();
}

bool CodeEditWidget::askForSave() {
    if (!modified) {
        return true;
    }
    // if the content is modified, ask for save
    QMessageBox::StandardButton reply =
            QMessageBox::question(this, tr("保存文件"), tr("文件已修改，是否保存？"),
                                  QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    if (reply == QMessageBox::Yes) {
        saveFile();
    } else if (reply == QMessageBox::Cancel) {
        return false;
    }
    return true;
}

QCoro::Task<> CodeEditWidget::onTextChanged() {
    // This is a hack!
    // If highlighter has not changed the text, we should not emit modify signal
    if (highlighter->textNotChanged) {
        highlighter->textNotChanged = false;
        co_return;
    }

    if (!modified) {
        modified = true;
        emit modify();
    }
    if (requireCompletion) {
        co_await askForCompletion();
    }
    updateCompletionList();
    co_return;
}

/* Code tab widget */

CodeTabWidget::CodeTabWidget(QWidget *parent) : QTabWidget(parent) {
    setup();
    welcome();
    connect(this, &QTabWidget::tabCloseRequested, this, &CodeTabWidget::removeCodeEditRequested);
    connect(this, &QTabWidget::currentChanged, this, &CodeTabWidget::onCurrentTabChanged);
}

void CodeTabWidget::setProject(Project *project) {
    this->project = project;
    clearAll();
}

void CodeTabWidget::clearAll() {
    for (int i = count() - 1; i >= 0; --i) {
        removeTab(i);
    }
    welcome();
}

void CodeTabWidget::setup() {
    setTabsClosable(true);
    setMovable(true);
    setStyleSheet(loadText("qss/code.css"));
}

CodeEditWidget *CodeTabWidget::curEdit() const {
    return qobject_cast<CodeEditWidget *>(currentWidget());
}
CodeEditWidget *CodeTabWidget::editAt(int index) const {
    return qobject_cast<CodeEditWidget *>(widget(index));
}

void CodeTabWidget::welcome() { addTab(new WelcomeWidget(this), "欢迎"); }

void CodeTabWidget::addCodeEdit(const QString &filePath) {
    // find if the file is already opened
    for (int i = 0; i < count(); ++i) {
        auto *edit = editAt(i);
        if (edit && edit->getFile().filePath() == filePath) {
            setCurrentIndex(i); // switch to the existing tab
            return;
        }
    }

    auto *edit = new CodeEditWidget(filePath, this);
    int index = addTab(edit, edit->getTabText());
    connect(edit, &CodeEditWidget::modify, this, [this, index] { widgetModified(index); });
    setCurrentIndex(index);
}

void CodeTabWidget::checkRemoveCodeEdit(const QString &filename) {
    for (int i = 0; i < count(); ++i) {
        auto *edit = editAt(i);
        if (edit && edit->getFile().filePath() == filename) {
            removeCodeEdit(i);
            return;
        }
    }
}

void CodeTabWidget::handleFileOperation(const QString &filename, FileOperation operation) {
    switch (operation) {
        case OPEN:
            addCodeEdit(filename);
            break;
        case RENAME:
        case DELETE:
            checkRemoveCodeEdit(filename);
        default:
            break;
    }
}

void CodeTabWidget::removeCodeEditRequested(int index) {
    if (index < 0 || index >= count())
        return;

    auto *edit = editAt(index);
    if (!edit || edit->askForSave()) {
        removeCodeEdit(index);
    }
}

void CodeTabWidget::widgetModified(int index) {
    // add a * after the title
    setTabText(index, editAt(index)->getTabText() + " *");
}

void CodeTabWidget::removeCodeEdit(int index) {
    if (index < 0 || index >= count())
        return;

    QWidget *w = widget(index);
    removeTab(index);
    w->deleteLater();

    if (count() == 0) {
        welcome();
    }
}


LangFileInfo CodeTabWidget::currentFile() const {
    if (auto *edit = curEdit()) {
        return edit->getFile();
    }
    return LangFileInfo::empty();
}

void CodeTabWidget::save() {
    if (auto *edit = curEdit()) {
        edit->saveFile();
        // recover the tab title
        setTabText(currentIndex(), edit->getTabText());
    }
}

void CodeTabWidget::onCurrentTabChanged(int) const {
    const auto *edit = curEdit();
    FooterWidget::instance().setFileLabel(edit ? edit->getFile().filePath() : "");
}
