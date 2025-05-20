#ifndef AI_ASSISTANT_H
#define AI_ASSISTANT_H

#include <QApplication>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QMutex>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollBar>
#include <QString>
#include <QTextBrowser>
#include <QTextEdit>
#include <QUrl>
#include <QVBoxLayout>
#include <qcorotask.h>
#include "../ide/aiChat.h"
#include "../web/aiClient.h"
#include "../web/parse.h"
#include "../widgets/preview.h"

// Forward declaration
class CodeTabWidget;

class AIAssistantWidget : public QDockWidget {
    Q_OBJECT

public:
    explicit AIAssistantWidget(CodeTabWidget *codeTab, QWidget *parent = nullptr);
    ~AIAssistantWidget() override;

    void setUserCode(const QString &code) const;

    // Display current problem information
    void showCurrentProblemInfo();

private slots:
    void onSendClicked() const;
    void onAnalyzeClicked();
    void onCodeClicked();
    void onDebugClicked();
    void onClearClicked() const;
    void onInsertCodeClicked();
    void onSetApiKeyClicked();
    void onShowProblemClicked();
    void onMessageAdded(const AIMessage &message);
    void onRequestCompleted(bool success, const QString &response);
    void onTextChanged() const;
    
public:
    // Actively get problem information
    bool tryGetProblemInfo();

private:
    void setup();
    void connectSignals();
    void displayMessage(const AIMessage &message);
    void displayMarkdown(const QString &text, AIMessageType role);
    void setProgressVisible(bool visible) const;
    static QString extractCodeFromMarkdown(const QString &markdown);
    QString getCurrentCode() const;
    void insertGeneratedCode(const CodeTabWidget *codeTabWidget);
    bool getProblemInfoFromPreview(const OpenJudgePreviewWidget *preview);
    QString getFullProblemDescription() const;

    // Helper function to send AI request
    void sendAIRequest(const QString &prompt, const QString &requestType) const;

    // Debug logging helper functions
    static void logDebug(const QString &message);
    void logCurrentProblemInfo() const;

    // UI components
    QWidget *mainWidget = nullptr;
    QVBoxLayout *mainLayout = nullptr;
    QHBoxLayout *buttonLayout = nullptr;
    QHBoxLayout *settingsLayout = nullptr;
    QLabel *apiKeyLabel = nullptr;

    QTextBrowser *conversationView = nullptr;
    QTextEdit *userInput = nullptr;
    QPushButton *sendButton = nullptr;
    QPushButton *clearButton = nullptr;
    QPushButton *analyzeButton = nullptr;
    QPushButton *codeButton = nullptr;
    QPushButton *debugButton = nullptr;
    QPushButton *insertCodeButton = nullptr;
    QPushButton *showProblemButton = nullptr;
    QPushButton *setApiKeyButton = nullptr;
    QProgressBar *progressBar = nullptr;

    // Data members
    CodeTabWidget *codeTab = nullptr;
    OJProblemDetail problem;
    QString currentUserCode;
    QString generatedCode;
    QString historyMarkdown;
};

#endif // AI_ASSISTANT_H
