#ifndef ICON_NAV_H
#define ICON_NAV_H

#include <QFrame>
#include <QPushButton>
#include <QVBoxLayout>

class IconNavigateWidget : public QFrame {
    Q_OBJECT

    void setup();

protected:
    QVBoxLayout *layout;
    QList<QPushButton *> buttons;

    QPushButton *newIcon(const QString &iconPath, const QString &tooltip, bool toggle = true);

public:
    explicit IconNavigateWidget(QWidget *parent = nullptr);
};

class LeftIconNavigateWidget : public IconNavigateWidget {
    Q_OBJECT

    void addIcons();

private slots:
    void onToggleFileTree(bool checked);
    void onToggleTerminal(bool checked);

public:
    explicit LeftIconNavigateWidget(QWidget *parent = nullptr);

signals:
    void toggleFileTree(bool show);
    void toggleTerminal(bool show);
};

class RightIconNavigateWidget : public IconNavigateWidget {
    Q_OBJECT

    void addIcons();

private slots:
    void onTogglePreview(bool checked);
    void onOpenSetting();

public:
    explicit RightIconNavigateWidget(QWidget *parent = nullptr);

signals:
    void togglePreview(bool show);
    void openSetting();
};

#endif // ICON_NAV_H
