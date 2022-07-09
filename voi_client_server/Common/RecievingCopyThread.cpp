#include "RecievingCopyThread.h"
#include "QFile"
#include "QDebug"
#include <QTimer>

void RecievingCopyThread::doCopy(QByteArray arr){
    int nProcent = 0, n_ProcentOld = 0, nNumbBytesWrite = 0;
    if(arr.size()){
        nNumbBytesWrite = m_fileTo.write(arr);
        if(nNumbBytesWrite < 0){// какая то ошибка
            emit copyStatus(LocalCopyThread::WriteFileError);
            emit copyIsFinishing(LocalCopyThread::WriteFileError);
            return;
        }
        m_fileTo.flush();
        nProcent = (int)((m_fileTo.size())/(float)m_fileSize * 100.f);
        if(nProcent != n_ProcentOld){
            n_ProcentOld = nProcent;
        }
        emit procentReg(nProcent);
        emit sendNextCopyPack();
    }
}
//---------------------------------------------------------------------------------
void RecievingCopyThread::cancelCopying()
{
    emit copyStatus(LocalCopyThread::CopyisCanceled);
    if(m_fileTo.exists()) m_fileTo.close();
    emit copyIsFinishing(LocalCopyThread::CopyisCanceled);
}
//-------------------------------------------------------------------------
void RecievingCopyThread::stopCopying(int status)
{
    emit copyStatus(status);
    if(m_fileTo.exists()) m_fileTo.close();
    emit copyIsFinishing(status);
}
