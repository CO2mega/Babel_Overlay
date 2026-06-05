#ifndef SUBTITLEWIDGET_H
#define SUBTITLEWIDGET_H

#include <QWidget>
#include "scrollingtext.h"

class SubtitleWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SubtitleWidget(QWidget *parent = nullptr);

    void setOriginalText(const QString &text);
    void setTranslation(const QString &text);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    ScrollingText *m_originalText;
    ScrollingText *m_translation;
};

#endif // SUBTITLEWIDGET_H
