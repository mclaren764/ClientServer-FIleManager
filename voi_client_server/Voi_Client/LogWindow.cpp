#include "LogWindow.h"
#include "VoiTypes.h"

#include <QSettings>
#include <QGuiApplication>
#include <QScreen>
#include <QMenu>
#include <QMouseEvent>
#include <QDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QDateTime>
#include <QFile>
#include <QMessageBox>
#include <QDebug>

LogWindow::LogWindow(QWidget* parent) : QTreeWidget(parent)
{
    m_strFilterHide = m_strFilterShow = "";
    m_itemCount = 0;
    QFont oFnt = font();
    oFnt.setPointSize(8);
    setFont(oFnt);

    m_filterColor[e_Info] = Qt::black;
    m_filterColor[e_Warn] = QColor(225, 125, 166);
    m_filterColor[e_Err] = Qt::red;
    m_filterColor[e_Debug] = Qt::blue;

    m_filterType[e_Info] = tr("Информация");
    m_filterType[e_Warn] = tr("Предупреждение");
    m_filterType[e_Err] = tr("Ошибка");
    m_filterType[e_Debug] = tr("Отладка");

    QPixmap oPix(16, 16);
    for(int i = 0; i < e_MaxFiltr; i++) {
        oPix.fill(m_filterColor[i]);
        m_filterIcon[i] = new QIcon(oPix);
    }

    setWindowFlags(Qt::WindowStaysOnTopHint);
    setWindowTitle(tr("Журнал сообщений"));
    setHeaderLabels(QStringList()<<"время"<<"модуль"<<"сообщение");

    QSettings oSet("GraphicParam.ini", QSettings::IniFormat);
    QPoint pos = oSet.value("LogWndPos", QPoint(200, 200)).toPoint();
    QSize size = oSet.value("LogWndSize", QSize(400, 400)).toSize();
    m_messFilterDisp = oSet.value("LogWndFiltr", 0xffff).toInt();
    m_isAuto = oSet.value("LogWndScroll", true).toBool();

    QRect rc = QGuiApplication::primaryScreen()->availableGeometry();
    if(!rc.contains(pos)) pos = QPoint(rc.width() / 3, rc.height() / 3);
    if(!rc.contains(QRect(pos, size))) {
        pos = QPoint(rc.width() / 3, rc.height() / 3);
        size = QSize(rc.width() / 3, rc.height() / 3);
    }
    resize(size);
    move(pos);
}

void LogWindow::reDraw()
{
    QTreeWidgetItem* pItem;
    for(int i = 0; i < m_itemCount; i++) {
      pItem = topLevelItem(i);
      if(!pItem) continue;
      checkItemShow(pItem);
      }
    if(m_isAuto) scrollToBottom();
}
//---------------------------------------------------------
void LogWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() & Qt::RightButton) {
        QMenu* pMenu = new QMenu;
        QAction *pAct;
        for(int i = 0; i < e_MaxFiltr; i++) {
            pAct = pMenu->addAction(*(m_filterIcon[i]), m_filterType[i], [this, i](){
                if(m_messFilterDisp & (0x1<<i)) m_messFilterDisp &= ~(0x1<<i);
                else m_messFilterDisp |= (0x1<<i);
                reDraw();
            });
            pAct->setCheckable(true);
            pAct->setChecked(m_messFilterDisp & (0x1<<i));
        }

        pMenu->addSeparator();
        pAct = pMenu->addAction(tr("Отображать только сообщение"), [this](){
            QDialog *pDlg = new QDialog(this);
            pDlg->setWindowTitle(tr("Задание фильтра для отображения сообщений"));
            QVBoxLayout *pVL = new QVBoxLayout;
            QLineEdit* pLE = new QLineEdit;
            pVL->addWidget(pLE);
            QHBoxLayout* pHL = new QHBoxLayout;
            QPushButton *pOk = new QPushButton(tr("Ок"));
            pHL->addWidget(pOk);
            connect(pOk, &QPushButton::clicked, pDlg, &QDialog::accept);
            QPushButton *pCanc = new QPushButton(tr("Отмена"));
            pHL->addWidget(pCanc);
            connect(pCanc, &QPushButton::clicked, pDlg, &QDialog::reject);
            pVL->addLayout(pHL);
            pDlg->setLayout(pVL);
            if(pDlg->exec() == QDialog::Accepted) {
                m_strFilterShow = pLE->text();
                reDraw();
            }
            pDlg->deleteLater();
        });
        pAct = pMenu->addAction(tr("Не отображать сообщение"), [this](){
            QDialog *pDlg = new QDialog(this);
            pDlg->setWindowTitle(tr("Задание фильтра для скрытия сообщения"));
            QVBoxLayout *pVL = new QVBoxLayout;
            QLineEdit* pLE = new QLineEdit;
            pVL->addWidget(pLE);
            QHBoxLayout* pHL = new QHBoxLayout;
            QPushButton *pOk = new QPushButton(tr("Ок"));
            pHL->addWidget(pOk);
            connect(pOk, &QPushButton::clicked, pDlg, &QDialog::accept);
            QPushButton *pCanc = new QPushButton(tr("Отмена"));
            pHL->addWidget(pCanc);
            connect(pCanc, &QPushButton::clicked, pDlg, &QDialog::reject);
            pVL->addLayout(pHL);
            pDlg->setLayout(pVL);
            if(pDlg->exec() == QDialog::Accepted) {
                m_strFilterHide = pLE->text();
                reDraw();
            }
            pDlg->deleteLater();
        });
        pAct = pMenu->addAction(tr("Сброс фильтров по сообщенииям"), [this](){
            m_strFilterHide = m_strFilterShow = "";
            reDraw();
        });

        pMenu->addSeparator();
        pAct = pMenu->addAction(tr("Автоматический переход на посл. собщение"), [this](){
            m_isAuto = !m_isAuto;
        });
        pAct->setCheckable(true);
        pAct->setChecked(m_isAuto);
        pMenu->exec(mapToParent(event->pos()));
        delete (pMenu);
    }
}
//---------------------------------------------------------
void LogWindow::checkItemShow(QTreeWidgetItem* pItem)
{
    if(m_messFilterDisp & (0x1<<pItem->data(0, Qt::UserRole).toInt())) {
        if(m_strFilterShow.isEmpty()) { // фильтр на отображение не задан
            if(m_strFilterHide.isEmpty()) pItem->setHidden(false); // фильтр на скрытие не задан
            else { // фильтр на скрытие задан
                if(pItem->text(e_Text).contains(m_strFilterHide)) pItem->setHidden(true);
                else pItem->setHidden(false);
            } // фильтр на скрытие задан
        } // фильтр на отображение не задан
        else { // фильтр на отображение задан
            if(pItem->text(e_Text).contains(m_strFilterShow)) {
                if(m_strFilterHide.isEmpty()) pItem->setHidden(false); // фильтр на скрытие не задан
                else { // фильтр на скрытие задан
                    if(pItem->text(e_Text).contains(m_strFilterHide)) pItem->setHidden(true);
                    else pItem->setHidden(false);
                } // фильтр на скрытие задан
            }
            else pItem->setHidden(true);
        } // фильтр на отображение задан
    }
    else pItem->setHidden(true);
}

void LogWindow::displayOpenedLog(QString fileName)
{
    setWindowTitle(fileName.right(fileName.length() - fileName.lastIndexOf("/")-1));
    m_file.setFileName(fileName);
    if(!m_file.open( QIODevice::ReadOnly)) {
      QMessageBox::warning(this, tr("Сообщение ВОИ"), tr(" Невозможно открыть файл для просмотра"));
      close();
      return;
      }
    char str[1024];
    int nMax = 1000000; // максимальное количество строк
    //int nMaxLen = 0; // максимальная длина строки
    LogMessage oMess;
    while(nMax){
      nMax--;
      int nLen = m_file.readLine(str, 1024);
      if(nLen < 0) break; // конец файла
      // Удаляем символ \n  или \r из конца строки
      int nLast = nLen - 1;
      if((str[nLast] == '\n') || (str[nLast] == '\r')) str[nLast] = 0;
      // В Win в текстовый файл добавляется еще и '\r', в Линуксе только \n
      nLast = nLen - 2;
      // Удаляем символ \n  или \r из конца строки
      if((str[nLast] == '\n') || (str[nLast] == '\r')) str[nLast] = 0;

      //if(nMaxLen < nLen) nMaxLen = nLen;
      QString str1 = str;
      QString timeMes = str1.mid(str1.indexOf("m")+1, str1.indexOf("{")-str1.indexOf("m")-1);
      QString name = str1.mid(str1.indexOf("  ")+2, str1.indexOf("  ", str1.indexOf("  ")+2)-str1.indexOf("  ")-1);
      oMess.m_message = str1.mid(str1.indexOf("  ", str1.indexOf("  ")+2)+2, str1.indexOf("[m"));
      oMess.m_header.m_messageType = str1.mid(str1.indexOf("{")+1, 1).toUInt();
//      for (int i;i<5000 ;i++ ) {
//          qDebug()<<"Spamming";

//      }
      onMess(oMess, name, timeMes);
      }

    }
//---------------------------------------------------------
void LogWindow::closeEvent(QCloseEvent* )
{
    QSettings oSet("GraphicParam.ini", QSettings::IniFormat);
    oSet.setValue("LogWndPos", pos());
    oSet.setValue("LogWndSize", size());
    oSet.setValue("LogWndFiltr", m_messFilterDisp);
    oSet.setValue("LogWndScroll", m_isAuto);
    oSet.sync();
    emit closed();
}
//---------------------------------------------------------
void LogWindow::onMess(const LogMessage oMess, const QString sName, const QString time) // прием сообщения
{
    //static const int nItemCountMax = 2000;
    QFont serifFont("Times", 15, QFont::Thin);
    QFont sansFont("Helvetica [Cronyx]", 12);

    QTreeWidgetItem* pItem = new QTreeWidgetItem(this);
    m_itemCount++;
    pItem->setText(e_Time, time);
    pItem->setText(e_ID, tr("%1").arg(sName));
    pItem->setText(e_Text, oMess.m_message);
    uint nType = oMess.m_header.m_messageType;
    if(nType >= e_MaxFiltr) nType = 0;

    pItem->setData(0, Qt::UserRole, nType);
    pItem->setTextColor(e_Text, m_filterColor[nType]);

    pItem->setFont(e_Time, sansFont);
    pItem->setFont(e_ID, serifFont);
    pItem->setFont(e_Text, serifFont);


    checkItemShow(pItem);
    if(m_isAuto) scrollToItem(pItem, QAbstractItemView::PositionAtBottom);

//    if(n_ItemCount > nItemCountMax) {
//        int nDelCnt = 100;
//        int nCntPr = n_ItemCount;
//        for(int i = 0; i < nDelCnt; i++) delete takeTopLevelItem(0);
//        n_ItemCount = topLevelItemCount(); // чтоб нигде не ошибиться
//        ToLogFileDeb(tr("Очистка окна логов c %1  %2").arg(nCntPr).arg(n_ItemCount));
//    }
}
