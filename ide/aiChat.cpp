#include "aiChat.h"

// Initialize static instance pointer
AIChatManager *AIChatManager::instance = nullptr;

AIChatManager::AIChatManager(QObject *parent) : QObject(parent) {
    maxContextLength = 10; // Default: keep the last 10 messages as context
}

AIChatManager &AIChatManager::getInstance() {
    if (!instance) {
        instance = new AIChatManager();
    }
    return *instance;
}

void AIChatManager::addMessage(AIMessageType type, const QString &content) {
    AIMessage message(type, content);
    messages.append(message);
    emit messageAdded(message);
}

QList<AIMessage> AIChatManager::getMessages() const { return messages; }

QString AIChatManager::getContext(int maxTokens) const {
    QString context;

    // Collect context starting from the most recent messages
    for (int i = messages.size() - 1; i >= 0; i--) {
        const AIMessage &msg = messages[i];
        QString role;

        switch (msg.type) {
            case AIMessageType::USER:
                role = "User";
                break;
            case AIMessageType::ASSISTANT:
                role = "Assistant";
                break;
            case AIMessageType::SYSTEM:
                role = "System";
                break;
        }

        QString messageText = role + ": " + msg.content + "\n\n";

        // Simple token count estimation
        if (context.length() + messageText.length() > maxTokens * 2) {
            break;
        }

        context = messageText + context;
    }

    return context;
}

void AIChatManager::clearMessages() { messages.clear(); }
