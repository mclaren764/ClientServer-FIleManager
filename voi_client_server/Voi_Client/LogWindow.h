#ifndef LOGWINDOW_H
#define LOGWINDOW_H

#include <QWidget>
#include <QTreeWidget>
#include "VoiTypes.h"
#include <QFile>

struct LogMessage;

class LogWindow : public QTreeWidget
{
    Q_OBJECT
    enum {e_Time, e_ID, e_Text};
    enum { e_Info, e_Warn, e_Err, e_Debug, e_MaxFiltr};
    bool m_isAuto; // автоскроллинг к самому низу файла
    int m_messFilterDisp; // фильтр на отображение сообщений
    int m_itemCount; // количество строк
    QIcon* m_filterIcon[e_MaxFiltr];
    QColor m_filterColor[e_MaxFiltr];
    QString m_filterType[e_MaxFiltr];
    QString m_strFilterShow, m_strFilterHide;
    QFile m_file;
protected:
    void mouseReleaseEvent(QMouseEvent *event);
    void closeEvent(QCloseEvent *);
signals:
    void closed();


public:
    LogWindow(QWidget* parent = 0);
    void reDraw();
    void checkItemShow(QTreeWidgetItem* pItem);
    void displayOpenedLog(const QString fileName);
    void onMess(const LogMessage oMess, const QString sName, const QString time);
};

#endif // LOGWINDOW_H
