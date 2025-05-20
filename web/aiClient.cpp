#include "aiClient.h"

#include <QDebug>
#include <QJsonArray>
#include <QNetworkReply>
#include <QNetworkRequest>

#include "../util/file.h"

// Initialize static instance pointer
AIClient *AIClient::instance = nullptr;

AIClient::AIClient(QObject *parent) : QObject(parent) {
    apiEndpoint = "https://api.deepseek.com/v1/chat/completions";
    apiKey = Configs::instance().get("aiAssistant.apiKey").toString();
}

AIClient &AIClient::getInstance() {
    if (!instance) {
        instance = new AIClient();
    }
    return *instance;
}

void AIClient::setApiKey(const QString &key) {
    apiKey = key;
    Configs::instance().set("aiAssistant.apiKey", key);
}

bool AIClient::hasApiKey() const { return !apiKey.isEmpty(); }

QJsonObject AIClient::buildRequestJson(const QString &prompt, int maxTokens, double temperature) {
    QJsonObject message;
    message["role"] = "user";
    message["content"] = prompt;

    QJsonArray messages;
    messages.append(message);

    QJsonObject json;
    json["model"] = "deepseek-coder"; // Using DeepSeek Coder model
    json["messages"] = messages;
    json["max_tokens"] = maxTokens;
    json["temperature"] = temperature;

    return json;
}

QCoro::Task<std::expected<QString, QString>>
AIClient::sendRequest(const QString &prompt, int maxTokens, double temperature) {
    if (!hasApiKey()) {
        co_return std::unexpected("API key not set");
    }

    QNetworkRequest request;
    request.setUrl(QUrl(apiEndpoint));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(apiKey).toUtf8());

    QJsonObject requestJson = buildRequestJson(prompt, maxTokens, temperature);
    QByteArray requestData = QJsonDocument(requestJson).toJson();

    QNetworkReply *reply = co_await nam.post(request, requestData);

    if (reply->error()) {
        QString errorMsg = reply->errorString();
        delete reply;
        co_return std::unexpected(errorMsg);
    }

    QByteArray responseData = reply->readAll();
    delete reply;

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        co_return std::unexpected("JSON parse error: " + parseError.errorString());
    }

    QJsonObject responseJson = doc.object();

    // Check for errors
    if (responseJson.contains("error")) {
        QString errorMsg = responseJson["error"].toObject()["message"].toString();
        co_return std::unexpected(errorMsg);
    }

    // Extract generated text
    QJsonArray choices = responseJson["choices"].toArray();
    if (choices.isEmpty()) {
        co_return std::unexpected("No content in API response");
    }

    QString responseText = choices[0].toObject()["message"].toObject()["content"].toString();
    emit requestCompleted(true, responseText);

    co_return responseText;
}

void AIClient::sendRequestSync(const QString &prompt, int maxTokens, double temperature) {
    if (!hasApiKey()) {
        emit requestCompleted(false, "API key not set");
        return;
    }

    QNetworkRequest request;
    request.setUrl(QUrl(apiEndpoint));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(apiKey).toUtf8());

    QJsonObject requestJson = buildRequestJson(prompt, maxTokens, temperature);
    QByteArray requestData = QJsonDocument(requestJson).toJson();

    QNetworkReply *reply = nam.post(request, requestData);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error()) {
            QString errorMsg = reply->errorString();
            emit requestCompleted(false, errorMsg);
            reply->deleteLater();
            return;
        }

        QByteArray responseData = reply->readAll();
        reply->deleteLater();

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            emit requestCompleted(false, "JSON parse error: " + parseError.errorString());
            return;
        }

        QJsonObject responseJson = doc.object();

        // Check for errors
        if (responseJson.contains("error")) {
            QString errorMsg = responseJson["error"].toObject()["message"].toString();
            emit requestCompleted(false, errorMsg);
            return;
        }

        // Extract generated text
        QJsonArray choices = responseJson["choices"].toArray();
        if (choices.isEmpty()) {
            emit requestCompleted(false, "No content in API response");
            return;
        }

        QString responseText = choices[0].toObject()["message"].toObject()["content"].toString();
        emit requestCompleted(true, responseText);
    });
}
