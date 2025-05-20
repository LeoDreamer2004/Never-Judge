#include "script.h"

#include <QProcess>
#include <qcoro/qcoroprocess.h>

#include "file.h"

ScriptResult ScriptResult::fail() { return {false}; }
ScriptResult ScriptResult::ok(int exitCode, const QString &stdOut, const QString &stdErr) {
    return {true, exitCode, stdOut, stdErr};
}

QCoro::Task<ScriptResult> runPythonScript(QFile &script, QStringList args) {
    if (!script.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to read script: " << script.fileName();
        co_return ScriptResult::fail();
    }
    QTextStream in(&script);
    QString content = in.readAll();

    auto process = QProcess();
    co_await qCoro(process).start("python", QStringList() << "-c" << content << args);
    if (!co_await qCoro(process).waitForStarted()) {
        qWarning() << "Failed to start script: " << process.errorString();
        co_return ScriptResult::fail();
    }
    if (!co_await qCoro(process).waitForFinished()) {
        qWarning() << "Script cost too long time: " << process.errorString();
        co_return ScriptResult::fail();
    }

    QString stdOut = process.readAllStandardOutput();
    QString stdErr = process.readAllStandardError();
    int exitCode = process.exitCode();
    co_return ScriptResult::ok(exitCode, stdOut, stdErr);
}
