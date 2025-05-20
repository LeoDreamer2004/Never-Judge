#include "aiAssistant.h"

#include <QFont>
#include <QFontMetrics>
#include <QInputDialog>
#include <QMessageBox>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextCursor>

#include "../ide/aiChat.h"
#include "../util/file.h"
#include "../web/aiClient.h"
#include "../widgets/code.h"

AIAssistantWidget::AIAssistantWidget(CodeTabWidget *codeTab, QWidget *parent) :
    QDockWidget(tr("AI 刷题助手"), parent), codeTab(codeTab) {
    logDebug("Initializing AI Assistant window");
    
    // 禁用浮动窗口功能
    setFeatures(features() & ~DockWidgetFloatable);

    setup();
    connectSignals();
    if (!codeTab) {
        logDebug("Code editor pointer is null");
    }
}

AIAssistantWidget::~AIAssistantWidget() {}

void AIAssistantWidget::setup() {
    // Load CSS stylesheet
    QString stylesheet = loadText("qss/aiAssistant.css");

    mainWidget = new QWidget(this);
    mainWidget->setObjectName("aiAssistantMainWidget");
    mainLayout = new QVBoxLayout(mainWidget);

    conversationView = new QTextBrowser(mainWidget);
    conversationView->setObjectName("conversationView");
    conversationView->setOpenLinks(false);
    conversationView->setOpenExternalLinks(true);
    conversationView->setTextInteractionFlags(Qt::TextBrowserInteraction);

    QFont font("Microsoft YaHei", 10);
    conversationView->setFont(font);

    userInput = new QTextEdit(mainWidget);
    userInput->setObjectName("userInput");
    userInput->setPlaceholderText(tr("输入你的问题..."));
    userInput->setMaximumHeight(100);
    userInput->setFont(font);

    buttonLayout = new QHBoxLayout();

    sendButton = new QPushButton(tr("发送"), mainWidget);
    clearButton = new QPushButton(tr("清空"), mainWidget);
    analyzeButton = new QPushButton(tr("题目解析"), mainWidget);
    codeButton = new QPushButton(tr("示例代码"), mainWidget);
    debugButton = new QPushButton(tr("调试"), mainWidget);
    showProblemButton = new QPushButton(tr("显示题目"), mainWidget);
    insertCodeButton = new QPushButton(tr("插入代码"), mainWidget);
    insertCodeButton->setVisible(false); // show when the code is available

    // Apply CSS classes to buttons
    sendButton->setProperty("class", "regularButton");
    clearButton->setProperty("class", "regularButton");
    analyzeButton->setProperty("class", "highlightButton");
    codeButton->setProperty("class", "highlightButton");
    debugButton->setProperty("class", "regularButton");
    showProblemButton->setProperty("class", "regularButton");
    insertCodeButton->setProperty("class", "regularButton");

    buttonLayout->addWidget(showProblemButton);
    buttonLayout->addWidget(analyzeButton);
    buttonLayout->addWidget(codeButton);
    buttonLayout->addWidget(debugButton);
    buttonLayout->addWidget(insertCodeButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(clearButton);
    buttonLayout->addWidget(sendButton);

    settingsLayout = new QHBoxLayout();

    apiKeyLabel = new QLabel(tr("API 密钥:"), mainWidget);
    apiKeyLabel->setObjectName("apiKeyLabel");
    setApiKeyButton = new QPushButton(tr("设置"), mainWidget);
    setApiKeyButton->setProperty("class", "regularButton");

    settingsLayout->addWidget(apiKeyLabel);
    settingsLayout->addWidget(setApiKeyButton);
    settingsLayout->addStretch();

    progressBar = new QProgressBar(mainWidget);
    progressBar->setObjectName("progressBar");
    progressBar->setRange(0, 0);
    progressBar->setVisible(false);

    mainLayout->addLayout(settingsLayout);
    mainLayout->addWidget(conversationView);
    mainLayout->addWidget(progressBar);
    mainLayout->addWidget(userInput);
    mainLayout->addLayout(buttonLayout);

    // Apply stylesheet to the widget
    mainWidget->setStyleSheet(stylesheet);

    setWidget(mainWidget);
    resize(400, 600);
}

void AIAssistantWidget::connectSignals() {
    connect(sendButton, &QPushButton::clicked, this, &AIAssistantWidget::onSendClicked);
    connect(clearButton, &QPushButton::clicked, this, &AIAssistantWidget::onClearClicked);
    connect(analyzeButton, &QPushButton::clicked, this, &AIAssistantWidget::onAnalyzeClicked);
    connect(codeButton, &QPushButton::clicked, this, &AIAssistantWidget::onCodeClicked);
    connect(debugButton, &QPushButton::clicked, this, &AIAssistantWidget::onDebugClicked);
    connect(showProblemButton, &QPushButton::clicked, this,
            &AIAssistantWidget::onShowProblemClicked);
    connect(setApiKeyButton, &QPushButton::clicked, this, &AIAssistantWidget::onSetApiKeyClicked);
    connect(insertCodeButton, &QPushButton::clicked, this, &AIAssistantWidget::onInsertCodeClicked);

    connect(&AIChatManager::getInstance(), &AIChatManager::messageAdded, this,
            &AIAssistantWidget::onMessageAdded);
    connect(&AIClient::getInstance(), &AIClient::requestCompleted, this,
            &AIAssistantWidget::onRequestCompleted);
    connect(userInput, &QTextEdit::textChanged, this, &AIAssistantWidget::onTextChanged);
}

QString AIAssistantWidget::getCurrentCode() const {
    logDebug("Attempting to get current code");

    if (!codeTab) {
        logDebug("CodeTabWidget pointer is null");
        qWarning() << "AIAssistantWidget: CodeTabWidget pointer is null.";
        return {};
    }

    auto currentEdit = codeTab->curEdit();
    if (!currentEdit) {
        logDebug("No active code editor found");
        qWarning() << "AIAssistantWidget: No active code editor found.";
        return {};
    }

    QString code = currentEdit->toPlainText();
    logDebug("Successfully retrieved current code, length: " + QString::number(code.length()) +
             " characters");
    return code;
}

void AIAssistantWidget::displayMessage(const AIMessage &message) {
    QString text = message.content;
    displayMarkdown(text, message.type);
}

void AIAssistantWidget::displayMarkdown(const QString &text, AIMessageType role) {
    QString roleText =
            role == AIMessageType::USER
                    ? "<p style=\"color: #569CD6; font-weight: bold; font-size: large;\">用户</p>"
                    : "<p style=\"color: #C586C0; font-weight: bold; font-size: large;\">AI "
                      "助手</p>";
    historyMarkdown += roleText;
    historyMarkdown += "\n\n --- \n\n";
    historyMarkdown += text + "<br /><br /><br />";
    conversationView->setMarkdown(historyMarkdown);

    QScrollBar *scrollBar = conversationView->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void AIAssistantWidget::setProgressVisible(bool visible) const {
    progressBar->setVisible(visible);
    sendButton->setEnabled(!visible);
    clearButton->setEnabled(!visible);
    analyzeButton->setEnabled(!visible);
    codeButton->setEnabled(!visible);
    debugButton->setEnabled(!visible);
    userInput->setEnabled(!visible);
}

void AIAssistantWidget::setUserCode(const QString &code) const { userInput->setPlainText(code); }

void AIAssistantWidget::onSendClicked() const {
    QString message = userInput->toPlainText().trimmed();
    if (message.isEmpty()) {
        return;
    }

    // 检查API密钥是否设置（由于方法是const，我们不能直接调用非const的onSetApiKeyClicked()）
    if (!AIClient::getInstance().hasApiKey()) {
        logDebug("API key not set, cannot send message");
        QMessageBox::warning(nullptr, tr("API密钥未设置"), 
                           tr("请先设置DeepSeek API密钥才能使用AI助手功能。"));
        return;
    }

    AIChatManager::getInstance().addMessage(AIMessageType::USER, message);

    userInput->clear();
    setProgressVisible(true);
    sendAIRequest(message, "user information");
}

void AIAssistantWidget::onClearClicked() const {
    AIChatManager::getInstance().clearMessages();
    conversationView->clear();
}

bool AIAssistantWidget::tryGetProblemInfo() {
    logDebug("Attempting to actively get problem information");

    QWidget *parent = this;
    while (parent && !parent->inherits("IDEMainWindow")) {
        parent = parent->parentWidget();
    }

    if (!parent) {
        logDebug("Main window not found");
        return false;
    }

    auto *preview = parent->findChild<OpenJudgePreviewWidget *>();
    if (!preview || !preview->isVisible()) {
        logDebug("Preview window not visible or not found");
        return false;
    }

    if (!getProblemInfoFromPreview(preview)) {
        logDebug("Failed to get problem info from preview window");
        return false;
    }

    return true;
}

void AIAssistantWidget::onAnalyzeClicked() {
    qDebug() << "[AI Assistant Debug] Problem analysis button clicked";

    // 检查API密钥是否设置
    if (!AIClient::getInstance().hasApiKey()) {
        logDebug("API key not set, prompting user to set it first");
        onSetApiKeyClicked();
        if (!AIClient::getInstance().hasApiKey()) {
            return; // 用户取消了设置，直接返回
        }
    }

    tryGetProblemInfo();

    if (problem.title.isEmpty() || problem.description.isEmpty()) {
        logDebug("Problem information incomplete, cannot generate analysis");
        QMessageBox::warning(this, tr("缺少题目信息"), tr("请先打开一个题目或手动输入题目信息。"));
        return;
    }

    QString fullDescription = getFullProblemDescription();
    QString prompt = QString("请详细分析以下算法题目，包括以下内容：\n"
                             "1. 题目的核心问题和考点\n"
                             "2. 解题思路和算法策略\n"
                             "3. 可能的边界情况和注意事项\n"
                             "4. 时间复杂度和空间复杂度分析\n\n"
                             "题目：%1\n\n%2")
                             .arg(problem.title, fullDescription);
    AIChatManager::getInstance().addMessage(AIMessageType::SYSTEM, tr("正在生成题目解析..."));

    setProgressVisible(true);
    sendAIRequest(prompt, "题目解析");
}

void AIAssistantWidget::onCodeClicked() {
    // 检查API密钥是否设置
    if (!AIClient::getInstance().hasApiKey()) {
        logDebug("API key not set, prompting user to set it first");
        onSetApiKeyClicked();
        if (!AIClient::getInstance().hasApiKey()) {
            return; // 用户取消了设置，直接返回
        }
    }

    tryGetProblemInfo();

    if (problem.title.isEmpty() || problem.description.isEmpty()) {
        logDebug("Problem information incomplete, cannot generate code");
        QMessageBox::warning(this, tr("缺少题目信息"), tr("请先打开一个题目或手动输入题目信息。"));
        return;
    }

    QString fullDescription = getFullProblemDescription();
    QString prompt =
            QString("请为以下算法题目生成一个完整、高效、易于理解的C++示例代码。代码应该：\n"
                    "1. 包含详细的注释，解释关键步骤和算法思路\n"
                    "2. 处理各种边界情况\n"
                    "3. 使用合适的数据结构和算法\n"
                    "4. 遵循良好的编程实践\n\n"
                    "题目：%1\n\n%2")
                    .arg(problem.title, fullDescription);

    AIChatManager::getInstance().addMessage(AIMessageType::SYSTEM, tr("正在生成示例代码..."));

    setProgressVisible(true);
    sendAIRequest(prompt, "示例代码");
}

void AIAssistantWidget::onDebugClicked() {
    // 检查API密钥是否设置
    if (!AIClient::getInstance().hasApiKey()) {
        logDebug("API key not set, prompting user to set it first");
        onSetApiKeyClicked();
        if (!AIClient::getInstance().hasApiKey()) {
            return; // 用户取消了设置，直接返回
        }
    }

    QString userCode = getCurrentCode();
    if (userCode.isEmpty()) {
        logDebug("No code retrieved");
        QMessageBox::warning(this, tr("缺少代码"), tr("请先打开或编写需要调试的代码。"));
        return;
    }

    tryGetProblemInfo();

    if (problem.title.isEmpty() || problem.description.isEmpty()) {
        logDebug("Problem information incomplete, cannot generate debug analysis");
        QMessageBox::warning(this, tr("缺少题目信息"), tr("请先打开一个题目或手动输入题目信息。"));
        return;
    }

    QString fullDescription = getFullProblemDescription();
    QString prompt = QString("请帮助调试以下代码，详细分析可能存在的问题：\n"
                             "1. 逻辑错误\n"
                             "2. 边界情况处理\n"
                             "3. 算法实现问题\n"
                             "4. 性能优化建议\n\n"
                             "题目：%1\n\n%2\n\n"
                             "代码：\n```cpp\n%3\n```")
                             .arg(problem.title, fullDescription, userCode);

    AIChatManager::getInstance().addMessage(AIMessageType::USER, "请帮我调试这段代码");

    setProgressVisible(true);
    sendAIRequest(prompt, "debug");
}

void AIAssistantWidget::onShowProblemClicked() {
    logDebug("Show problem button clicked");

    if (tryGetProblemInfo()) {
        logDebug("Successfully retrieved problem info, preparing to display");
    } else {
        logDebug("Failed to get new problem info, using current info");
    }

    showCurrentProblemInfo();
}

void AIAssistantWidget::onInsertCodeClicked() {
    QWidget *parent = this;
    while (parent && !parent->inherits("IDEMainWindow")) {
        parent = parent->parentWidget();
    }

    if (!parent) {
        logDebug("Cannot find main window");
        QMessageBox::warning(this, tr("无法访问编辑器"), tr("无法找到主窗口，无法插入代码。"));
        return;
    }

    logDebug("Found main window, attempting to get code editor");

    auto *codeTab = parent->findChild<CodeTabWidget *>();
    if (!codeTab) {
        logDebug("无法找到代码编辑器");
        QMessageBox::warning(this, tr("无法访问编辑器"), tr("无法找到代码编辑器，无法插入代码。"));
        return;
    }

    insertGeneratedCode(codeTab);
}

void AIAssistantWidget::onSetApiKeyClicked() {
    bool ok;
    QString apiKey = QInputDialog::getText(
            this, tr("设置 API 密钥"), tr("请输入 DeepSeek API 密钥:"), QLineEdit::Normal,
            AIClient::getInstance().hasApiKey() ? "********" : "", &ok);

    if (ok && !apiKey.isEmpty()) {
        AIClient::getInstance().setApiKey(apiKey);
        QMessageBox::information(this, tr("API 密钥已设置"), tr("DeepSeek API 密钥已成功设置。"));
    }
}

void AIAssistantWidget::onMessageAdded(const AIMessage &message) { displayMessage(message); }

QString AIAssistantWidget::extractCodeFromMarkdown(const QString &markdown) {
    // C++ code match
    static QRegularExpression cppCodeBlockRegex(R"(```cpp\s*([\s\S]*?)```)");
    QRegularExpressionMatch cppMatch = cppCodeBlockRegex.match(markdown);

    if (cppMatch.hasMatch()) {
        QString code = cppMatch.captured(1).trimmed();
        return code;
    }

    // python code match
    static QRegularExpression pythonCodeBlockRegex(R"(```python\s*([\s\S]*?)```)");
    QRegularExpressionMatch pythonMatch = pythonCodeBlockRegex.match(markdown);

    if (pythonMatch.hasMatch()) {
        QString code = pythonMatch.captured(1).trimmed();
        return code;
    }

    // any code match
    static QRegularExpression anyCodeBlockRegex(R"(```\w*\s*([\s\S]*?)```)");
    QRegularExpressionMatch anyMatch = anyCodeBlockRegex.match(markdown);

    if (anyMatch.hasMatch()) {
        QString code = anyMatch.captured(1).trimmed();
        return code;
    }

    logDebug("No code block from AI response");
    return {};
}

void AIAssistantWidget::insertGeneratedCode(const CodeTabWidget *codeTabWidget) {
    if (generatedCode.isEmpty()) {
        QMessageBox::warning(this, tr("没有可插入的代码"),
                             tr("请先生成示例代码或从对话中选择代码。"));
        return;
    }

    CodeEditWidget *editor = codeTabWidget->curEdit();
    if (!editor) {
        QMessageBox::warning(this, tr("没有打开的编辑器"), tr("请先打开一个代码文件。"));
        return;
    }

    editor->insertPlainText(generatedCode);
    QMessageBox::information(this, tr("代码已插入"), tr("代码已成功插入到编辑器中。"));
}

void AIAssistantWidget::onRequestCompleted(bool success, const QString &response) {
    setProgressVisible(false);

    if (success) {
        QString code = extractCodeFromMarkdown(response);
        if (!code.isEmpty()) {
            logDebug("从响应中提取到代码，长度: " + QString::number(code.length()) + " 字符");
            generatedCode = code;
            insertCodeButton->setVisible(true);
        } else {
            insertCodeButton->setVisible(false);
            generatedCode.clear();
        }

        AIChatManager::getInstance().addMessage(AIMessageType::ASSISTANT, response);
    } else {
        logDebug("Failed to get request for AI: " + response);

        QString errorMsg = tr("Error: ") + response;
        AIChatManager::getInstance().addMessage(AIMessageType::SYSTEM, errorMsg);

        insertCodeButton->setVisible(false);
        generatedCode.clear();
    }
}

// FIXME: Ctrl + Enter does not work
void AIAssistantWidget::onTextChanged() const {
    if (userInput->toPlainText().endsWith("\n") &&
        (QApplication::keyboardModifiers() & Qt::ControlModifier)) {
        QString text = userInput->toPlainText();
        text.chop(1);
        userInput->setPlainText(text);
    }
}

bool AIAssistantWidget::getProblemInfoFromPreview(const OpenJudgePreviewWidget *preview) {
    if (!preview) {
        logDebug("No preview widget");
        return false;
    }

    auto *titleLabel = preview->findChild<QLabel *>();
    if (!titleLabel) {
        logDebug("No title label in preview");
        return false;
    }

    QString title = titleLabel->text();
    if (title.isEmpty() || title == "题目预览") {
        logDebug("No title in preview");
        return false;
    }

    QList<QTextEdit *> textEdits = preview->findChildren<QTextEdit *>();
    if (textEdits.isEmpty()) {
        logDebug("Cannot find text editor");
        return false;
    }

    QString content;
    for (QTextEdit *textEdit: textEdits) {
        if (textEdit->isVisible()) {
            content = textEdit->toPlainText();
            break;
        }
    }

    if (content.isEmpty()) {
        for (QTextEdit *textEdit: textEdits) {
            if (textEdit->isVisible()) {
                content = textEdit->toHtml();
                break;
            }
        }
    }

    if (content.isEmpty()) {
        logDebug("Cannot find content in text editor");
        content = tr("题目内容无法直接获取");
    }

    QStringList lines = content.split("\n");
    QString description;
    QString inputDesc;
    QString outputDesc;
    QString sampleInput;
    QString sampleOutput;

    enum ParseState { None, Description, Input, Output, SampleInput, SampleOutput };

    ParseState state = None;

    for (const QString &line: lines) {
        if (line.contains("题目描述", Qt::CaseInsensitive)) {
            state = Description;
            continue;
        } else if (line.contains("输入", Qt::CaseInsensitive) &&
                   !line.contains("样例", Qt::CaseInsensitive)) {
            state = Input;
            continue;
        } else if (line.contains("输出", Qt::CaseInsensitive) &&
                   !line.contains("样例", Qt::CaseInsensitive)) {
            state = Output;
            continue;
        } else if (line.contains("样例输入", Qt::CaseInsensitive)) {
            state = SampleInput;
            continue;
        } else if (line.contains("样例输出", Qt::CaseInsensitive)) {
            state = SampleOutput;
            continue;
        }

        switch (state) {
            case Description:
                description += line + "\n";
                break;
            case Input:
                inputDesc += line + "\n";
                break;
            case Output:
                outputDesc += line + "\n";
                break;
            case SampleInput:
                sampleInput += line + "\n";
                break;
            case SampleOutput:
                sampleOutput += line + "\n";
                break;
            default:
                break;
        }
    }

    if (description.isEmpty()) {
        logDebug("Cannot find description, and use content as description.");
        description = content;
    }

    problem.title = std::move(title);
    problem.description = std::move(description.trimmed());
    problem.inputDesc = std::move(inputDesc.trimmed());
    problem.outputDesc = std::move(outputDesc.trimmed());
    problem.sampleInput = std::move(sampleInput.trimmed());
    problem.sampleOutput = std::move(sampleOutput.trimmed());

    logCurrentProblemInfo();

    return !problem.title.isEmpty() && !problem.description.isEmpty();
}

QString AIAssistantWidget::getFullProblemDescription() const {
    // Build complete problem information
    QString fullDescription = problem.description;

    if (!problem.inputDesc.isEmpty()) {
        fullDescription += "\n\nInput Description:\n" + problem.inputDesc;
    }
    if (!problem.outputDesc.isEmpty()) {
        fullDescription += "\n\nOutput Description:\n" + problem.outputDesc;
    }
    if (!problem.sampleInput.isEmpty()) {
        fullDescription += "\n\nSample Input:\n" + problem.sampleInput;
    }
    if (!problem.sampleOutput.isEmpty()) {
        fullDescription += "\n\nSample Output:\n" + problem.sampleOutput;
    }

    return fullDescription;
}

void AIAssistantWidget::showCurrentProblemInfo() {
    logDebug("Displaying current problem information");

    if (problem.title.isEmpty() || problem.description.isEmpty()) {
        logDebug("Problem information incomplete, cannot display");
        QMessageBox::warning(this, tr("题目信息不完整"), tr("当前没有完整的题目信息可以显示。"));
        return;
    }

    // Build problem information display text
    QString infoText = tr("当前题目信息：\n\n");
    infoText += tr("标题：%1\n\n").arg(problem.title);
    infoText += tr("描述：\n%1\n\n").arg(problem.description);

    if (!problem.inputDesc.isEmpty()) {
        infoText += tr("输入描述：\n%1\n\n").arg(problem.inputDesc);
    }

    if (!problem.outputDesc.isEmpty()) {
        infoText += tr("输出描述：\n%1\n\n").arg(problem.outputDesc);
    }

    if (!problem.sampleInput.isEmpty()) {
        infoText += tr("样例输入：\n%1\n\n").arg(problem.sampleInput);
    }

    if (!problem.sampleOutput.isEmpty()) {
        infoText += tr("样例输出：\n%1\n\n").arg(problem.sampleOutput);
    }

    // Display problem information
    AIChatManager::getInstance().addMessage(AIMessageType::SYSTEM, infoText);
}

void AIAssistantWidget::logDebug(const QString &message) {
    qDebug() << "[AI Assistant Debug]" << message;
}

void AIAssistantWidget::logCurrentProblemInfo() const {
    qDebug() << "[AI Assistant Debug] Current problem info:";
    qDebug() << "[AI Assistant Debug]   Title:"
             << (problem.title.isEmpty() ? "empty" : problem.title);
    qDebug() << "[AI Assistant Debug]   Description length:" << problem.description.length()
             << "characters";
    qDebug() << "[AI Assistant Debug]   Input description length:" << problem.inputDesc.length()
             << "characters";
    qDebug() << "[AI Assistant Debug]   Output description length:" << problem.outputDesc.length()
             << "characters";
    qDebug() << "[AI Assistant Debug]   Sample input length:" << problem.sampleInput.length()
             << "characters";
    qDebug() << "[AI Assistant Debug]   Sample output length:" << problem.sampleOutput.length()
             << "characters";
}

void AIAssistantWidget::sendAIRequest(const QString &prompt, const QString &requestType) const {
    qDebug() << "[AI Assistant Debug] Sending" << requestType
             << "request, length:" << prompt.length() << "characters";

    // 检查是否设置了API密钥
    if (!AIClient::getInstance().hasApiKey()) {
        logDebug("API key not set, cannot send request");
        setProgressVisible(false);
        QMessageBox::warning(nullptr, tr("API密钥未设置"), 
                             tr("请先设置DeepSeek API密钥才能使用AI助手功能。"));
        return;
    }

    // Use synchronous method to send request
    AIClient::getInstance().sendRequestSync(prompt);

    qDebug() << "[AI Assistant Debug]" << requestType << "request sent, waiting for callback";
}
