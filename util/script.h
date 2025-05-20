#ifndef SCRIPT_H
#define SCRIPT_H

#include <QFile>
#include <qcorotask.h>

struct ScriptResult {
    bool success;
    int exitCode;
    QString stdOut;
    QString stdErr;

    static ScriptResult fail();
    static ScriptResult ok(int exitCode, const QString &stdOut, const QString &stdErr);
};

QCoro::Task<ScriptResult> runPythonScript(QFile &script, QStringList args);

#endif // SCRIPT_H
