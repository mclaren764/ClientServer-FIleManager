#include "SendingCopyThread.h"
#include "QFile"
#include "QDebug"
#include <QTimer>

void SendingCopyThread::doSend(){
    int nNumb = 0;
    enum {kMaxBufferSize = 32 * 1024};
    QByteArray bytes;
    nNumb = (std::min)(static_cast<qint64>(kMaxBufferSize), m_fileSize);
    m_fileSize -= nNumb;
    if(nNumb){
      // Перепись из одной порции данных
      bytes = m_fileFrom.read(nNumb);
      if(bytes.size() < 0) {// какая то ошибка
          emit copyStatus(LocalCopyThread::ReadFileError);
          if(m_fileFrom.exists()) m_fileFrom.close();
          emit copyIsFinishing(LocalCopyThread::ReadFileError);
          return;
        }
      emit sendCopyPack(bytes);
    if(m_fileSize <= 0){
        if(m_fileFrom.exists()) m_fileFrom.close();
        emit copyStatus(LocalCopyThread::CopyisSuccessful);
        emit copyIsFinishing(LocalCopyThread::CopyisSuccessful);
    }
    }
}
//---------------------------------------------------------------------------------
void SendingCopyThread::cancelCopying()
{
    emit copyStatus(LocalCopyThread::CopyisCanceled);
    if(m_fileFrom.exists()) m_fileFrom.close();
    emit copyIsFinishing(LocalCopyThread::CopyisCanceled);
}
//-------------------------------------------------------------------------
void SendingCopyThread::stopCopying(int status)
{
    emit copyStatus(status);
    if(m_fileFrom.exists()) m_fileFrom.close();
    emit copyIsFinishing(status);
}
