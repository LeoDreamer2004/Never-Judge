#ifndef AICLIENT_H
#define AICLIENT_H

#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <expected>
#include <qcoro/qcoronetworkreply.h>
#include <qcorotask.h>

// DeepSeek API client class
class AIClient : public QObject {
    Q_OBJECT

private:
    QString apiKey;
    QString apiEndpoint;
    QNetworkAccessManager nam;

    // Singleton pattern
    explicit AIClient(QObject *parent = nullptr);
    static AIClient *instance;

public:
    // Get AIClient singleton instance
    static AIClient &getInstance();

    // Set the API key
    void setApiKey(const QString &key);

    // Check if the API key is set
    bool hasApiKey() const;

    // Send request to DeepSeek API (coroutine version)
    QCoro::Task<std::expected<QString, QString>>
    sendRequest(const QString &prompt, int maxTokens = 2048, double temperature = 0.7);

    // Send request to DeepSeek API (synchronous version)
    void sendRequestSync(const QString &prompt, int maxTokens = 2048, double temperature = 0.7);

    // Build API request JSON
    static QJsonObject buildRequestJson(const QString &prompt, int maxTokens, double temperature);

signals:
    // API call completed signal
    void requestCompleted(bool success, const QString &response);
};

#endif // AICLIENT_H
