#include "CopyDialog.h"
#include <QFile>
#include <unistd.h>
#include <HalfDlg.h>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileInfo>
#include <QObject>
#include <QTextCodec>
#include <LocalCopyThread.h>
#include <RecievingCopyThread.h>
#include <QJsonDocument>
#include <Common/Utils/AsyncFileUtils.h>

using namespace AsyncFileUtils;

//---------------------------------------------------------------------------------------------
CopyDialog::CopyDialog(FileManager *parent, uint type)
    :QDialog(parent, Qt::Dialog | Qt::WindowStaysOnTopHint)
{
    m_copyingType = type;
    m_cancelCopy = false;
    m_localCopyThread = nullptr;
    m_recievingCopier = nullptr;
    m_sendingCopier = nullptr;

    setModal(false);
    setMinimumSize(500,150);
    QVBoxLayout *vbl = new QVBoxLayout;
    QHBoxLayout *hbl1 = new QHBoxLayout;
    QHBoxLayout *hbl2 = new QHBoxLayout;
    QHBoxLayout *hbl3 = new QHBoxLayout;
    setWindowTitle("Копирование файла");
    // Ориентирование
    m_textLabelFrom = new QLabel("", this);
    m_textLabelTo = new QLabel("в:", this);
    m_textLabelClientServerFrom = new QLabel("Клиент", this);
    m_textLabelClientServerFrom->setMaximumSize(45,20);
    m_textLabelClientServerTo = new QLabel("Клиент",this);
    m_editLine = new QLineEdit("", this);
    m_okButton = new QPushButton("OK", this);
    m_okButton->setMaximumHeight(20);
    m_cancelButton = new QPushButton("Отмена", this);
    m_cancelButton->setMaximumHeight(20);
    m_copyProgressBar = new QProgressBar(this);
    m_copyProgressBar->setRange(0,100);
    m_copyProgressBar->setValue(0);
    hbl3->addWidget(m_textLabelClientServerFrom);
    hbl3->addWidget(m_textLabelFrom);
    vbl->addLayout(hbl3);
    hbl2->addWidget(m_textLabelClientServerTo);
    hbl2->addWidget(m_textLabelTo);
    hbl2->addWidget(m_editLine);
    vbl->addLayout(hbl2);
    hbl1->addWidget(m_okButton);
    hbl1->addWidget(m_cancelButton);
    vbl->addWidget(m_copyProgressBar);
    vbl->addLayout(hbl1);
    setLayout(vbl);
    // Обработчики событий
    connect(m_okButton, &QPushButton::clicked, this, [this](){
        accept();
        m_okButton->setDisabled(true);
        m_editLine->setDisabled(true);
    });
    connect(m_cancelButton, &QPushButton::clicked, this, [this](){
        reject();
    });
    showDlg();
}
CopyDialog::~CopyDialog(){
}
//----------------------------------------------------------------------------------------
void CopyDialog::showDlg()
{
    if(!m_editLine->isEnabled()) m_editLine->setEnabled(true);
    if(!m_okButton->isEnabled()) m_okButton->setEnabled(true);
    m_textLabelFrom->setText("Из: " + m_copyFrom);
    m_editLine->setText(m_copyTo);
    m_editLine->setFocus();
    m_copyProgressBar->setValue(0);
    show();
}
//--------------------------------------------------------------------------------------
void CopyDialog::onCopyThreadFinished(const QString filePath, int n_CopyStatus)
{
    bool needRemoveFile = false;
    switch (n_CopyStatus) {
    case 0:
        QMessageBox::information(this, tr("Сообщение программы ВОИ"), tr("Копирование успешно завершено"));
        break;
    case 1:
        if(QMessageBox::question(this,tr("Сообщение программы ВОИ"),tr("Сохранить скопированную часть?"),
                        QMessageBox::Yes, QMessageBox::No) == QMessageBox::No) needRemoveFile = true;
        break;
    case 2:
        QMessageBox::critical(this, tr("Ошибка копирование"), tr("Не открывается исходный файл копирования"));
        break;
    case 3:
        QMessageBox::critical(this, tr("Ошибка копирования"), tr("Не открывается конечный файл копирования"));
        break;
    case 4:
        QMessageBox::critical(this, tr("Ошибка копирования"), tr("Ошибка считывания информации из исходного файла"));
        break;
    case 5:
        QMessageBox::critical(this, tr("Ошибка копирования"), tr("Ошибка записи информации в файл назначения"));
        break;
    }

    if (needRemoveFile)
    {
        auto* futuгerWatcher = new QFutureWatcher<Error>(this);

        connect(futuгerWatcher, &QFutureWatcherBase::finished, this, [this, futuгerWatcher, filePath]() {
            const Error error = futuгerWatcher->result();
            const bool success = !error.isPresent();
            if (!success)
            {
                QMessageBox::critical(this, tr("Ошибка удаление файла"), error.message);
            }
            // Снятие блокировок кнопок, защита от закрытия окна
            setEnabled(true);
            if (success)
            {
                hide();
            }
            futuгerWatcher->deleteLater();
        });

        // Блокировка кнопок, защита от закрытия окна
        setEnabled(false);

        futuгerWatcher->setFuture(AsyncFileUtils::deleteFileAsync(filePath));
    }
    else
    {
        hide();
    }
}
//-------------------------------------------------------------------------------------
void CopyDialog::handleTheCopyStatus(int status)
{
    switch (m_copyingType) {
    case LocalCopyThread::ClientToServer:
        if(m_sendingCopier) {
            m_sendingCopier->deleteLater();
            m_sendingCopier = nullptr;
        }
        m_sendingCopyThread.quit();
        m_sendingCopyThread.wait();
        break;
    case LocalCopyThread::ServerToClient:
        if(m_recievingCopier) {
            m_recievingCopier->deleteLater();
            m_recievingCopier = nullptr;
        }
        m_recievingCopyThread.quit();
        m_recievingCopyThread.wait();
        break;
    case LocalCopyThread::ServerToServer:
        break;
    }
    onCopyThreadFinished(m_copyTo, status);

}
//--------------------------------------------------------------------------------------
void CopyDialog::startLocalThread()
{
    m_cancelCopy=0;
    m_localCopyThread = new LocalCopyThread(m_copyFrom, m_copyTo);
    connect(m_localCopyThread, &LocalCopyThread::procentReg, this,  &CopyDialog::setProcent);
    connect(this, &CopyDialog::cancelLocalCopy, m_localCopyThread, &LocalCopyThread::isCanceled);
    connect(m_localCopyThread, &LocalCopyThread::copyStatus, this, &CopyDialog::copyStatus);
    connect(m_localCopyThread, &LocalCopyThread::finished, this,  [this]() {
        emit copyIsFinished();
        m_localCopyThread->deleteLater();
        m_localCopyThread = nullptr;
        onCopyThreadFinished(m_copyTo, m_copyStatus);
        emit refreshCopyDlg();
    });
    m_localCopyThread->start();
}
//-------------------------------------------------------------------------------------------------
void CopyDialog::startRecievingThread(qint64 size)
{
    m_cancelCopy=0;
    m_recievingCopier = new RecievingCopyThread(m_copyTo, size);
    m_recievingCopier->moveToThread(&m_recievingCopyThread);
    connect(m_recievingCopier, &RecievingCopyThread::procentReg, this,  &CopyDialog::setProcent);
    connect(m_recievingCopier, &RecievingCopyThread::copyStatus, this, &CopyDialog::copyStatus);
    connect(this, &CopyDialog::finishTheCopy, m_recievingCopier, &RecievingCopyThread::stopCopying);
    connect(this, &CopyDialog::cancelTheCopy, m_recievingCopier, &RecievingCopyThread::cancelCopying);
    connect(m_recievingCopier, &RecievingCopyThread::sendNextCopyPack, this, [this](){
        QJsonObject obj
        {
            {"SendNext", true},
        };
        QJsonDocument doc = QJsonDocument(obj);
        emit readyToRecieveNext(doc.toJson());
    });
    connect(m_recievingCopier, &RecievingCopyThread::copyIsFinishing, this,  [this](int status) {
        emit finishServerToClientCopy(status);
        emit copyIsFinished();
    });
    m_recievingCopyThread.start();
    emit m_recievingCopier->sendNextCopyPack();
}
//---------------------------------------------------------------------------------------------------
void CopyDialog::startSendingThread()
{
    m_sendingCopier = new SendingCopyThread(m_copyFrom);
    m_sendingCopier->moveToThread(&m_sendingCopyThread);
    connect(m_sendingCopier, &SendingCopyThread::copyStatus, this, [this](uint status) {
                copyStatus(status);});
    connect(m_sendingCopier, &SendingCopyThread::sendFileSize, this, [this](qint64 fileSize){
        emit sendFileSize(fileSize, m_copyTo);
    });
    connect(m_sendingCopier, &SendingCopyThread::sendCopyPack, this, [this](QByteArray arr){
        emit sendCopyPack(arr);
    });
    connect(m_sendingCopier, &SendingCopyThread::copyIsFinishing, this,  [this](int copyStatus) {
        emit finishClientToServerCopy(copyStatus);
        emit copyIsFinished();
    });
    if(m_sendingCopier->getStatus() == LocalCopyThread::ReadFileCannotBeOpened){
        onCopyThreadFinished(m_copyTo, m_sendingCopier->getStatus());
        return;
    }
    m_sendingCopyThread.start();
    if(m_sendingCopier) m_sendingCopier->getFileSize();
}
//--------------------------------------------------------------------------------------------------
void CopyDialog::accept()
{
    if(!m_cancelButton->isEnabled()) m_cancelButton->setEnabled(true);
    m_copyTo = m_editLine->text();
    if(QFile::exists(m_copyTo)){
        if(QMessageBox::question(this, tr("Ошибка копирования"),
                                 tr("Файл с таким именем уже существует. Копировать файл ") + m_copyFrom + " в\n" + m_copyTo + " ?",
                                 QMessageBox::Yes, QMessageBox::No)==QMessageBox::No) {reject(); return;}
        else deleteFileAsync(m_copyTo);
    }
    switch (m_copyingType) {
    case LocalCopyThread::ClientToClient:
        startLocalThread();
        break;
    case LocalCopyThread::ClientToServer:
        startSendingThread();
        break;
    case LocalCopyThread::ServerToClient:
        emit serverCopy(m_copyFrom, m_copyTo, LocalCopyThread::ServerToClient);
        break;
    case LocalCopyThread::ServerToServer:
        emit serverCopy(m_copyFrom, m_copyTo, LocalCopyThread::ServerToServer);
        break;
    default:
        this->close();
        return;
        break;
    }
}
//----------------------------------------------------------------------------------------
void CopyDialog::reject() // отмена
{
    if(m_localCopyThread){
        if(m_localCopyThread->isRunning())
            emit cancelLocalCopy();
    }
    else if(m_sendingCopyThread.isRunning())
        emit cancelClientToServerCopy();
    else if(m_recievingCopyThread.isRunning())
        emit cancelServerToClientCopy();
    else
        emit cancelServerToServerCopy();
    done(true);
}
//-----------------------------------------------------------------------------------
