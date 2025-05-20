#ifndef AICHAT_H
#define AICHAT_H

#include <QDateTime>
#include <QList>
#include <QObject>
#include <QString>

// AI message type enum
enum class AIMessageType {
    USER, // User message
    ASSISTANT, // AI assistant message
    SYSTEM // System message
};

// AI message structure
struct AIMessage {
    AIMessageType type; // Message type
    QString content; // Message content
    QDateTime timestamp; // Timestamp

    AIMessage(AIMessageType type, const QString &content) :
        type(type), content(content), timestamp(QDateTime::currentDateTime()) {}
};

// AI chat manager class
class AIChatManager : public QObject {
    Q_OBJECT

    QList<AIMessage> messages; // Chat history
    int maxContextLength; // Maximum context length

    // Singleton pattern
    explicit AIChatManager(QObject *parent = nullptr);
    static AIChatManager *instance;

public:
    static AIChatManager &getInstance();

    /** Add a message to the manager */
    void addMessage(AIMessageType type, const QString &content);

    /** Get complete chat history */
    QList<AIMessage> getMessages() const;

    /** Get current context */
    QString getContext(int maxTokens = 4096) const;

    /** Clear chat history */
    void clearMessages();

signals:
    /** Message added signal */
    void messageAdded(const AIMessage &message);
};

#endif // AICHAT_H
