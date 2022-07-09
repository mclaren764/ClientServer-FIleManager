#include "LocalCopyThread.h"
#include <QFile>
//#include <QMessageBox>
//#include "CopyDialog.h"
#include <QTime>
#include <QDebug>

//-----------------------------------
void LocalCopyThread::run(){
    QFile p_FileFrom, p_FileTo;
    p_FileFrom.setFileName(m_copyFrom);
    p_FileTo.setFileName(m_copyTo);
        // Открыть исходный файл
    qint64 n_SizeFile = p_FileFrom.size();  // размер файла в байтах
    qint64 n_SizeFileBegin = n_SizeFile;  // в байтах
    if (!p_FileFrom.open(QIODevice::ReadOnly)){
        emit copyStatus(ReadFileCannotBeOpened);
        return;
    }
        // Открыть выходной файл
    if (!p_FileTo.open(QIODevice::WriteOnly)){
        emit copyStatus(WriteFileCannotBeOpened);
        return;
    }
    int nProcent, n_SizeFileTo = 0, n_ProcentOld = 0, nNumb = 0, nNumbBytesRead = 0, nNumbBytesWrite = 0;
    enum {kMaxBufferSize = 32 * 1024};
    char buf[kMaxBufferSize];
    while(isRunning()&&n_SizeFile!=0){
        nNumb = (std::min)(static_cast<qint64>(kMaxBufferSize), n_SizeFile);
        n_SizeFile -= nNumb;
        if(nNumb){
            // Перепись из одной порции данных
            nNumbBytesRead = p_FileFrom.read(buf, nNumb);
            if(nNumbBytesRead < 0){// какая то ошибка
                emit copyStatus(ReadFileError);
                return;
            }
            nNumbBytesWrite = p_FileTo.write(buf, nNumb);
            p_FileTo.flush();
            if(nNumbBytesWrite < 0){// какая то ошибка
                emit copyStatus(WriteFileError);
                return;
            }
            nProcent = (int)((p_FileTo.size())/(float)p_FileFrom.size() * 100.f);
            if(nProcent != n_ProcentOld){
                n_ProcentOld = nProcent;
            }
        }
        emit procentReg(n_ProcentOld);

        if(m_copyCancel == 1){
            emit copyStatus(CopyisCanceled);
            n_SizeFile = 0;// если нажали отмену то считаем, что весь файл скопирован
        }
        if(n_SizeFile <= 0) break;
        }
    if(p_FileFrom.exists()){
        p_FileFrom.close();
    }
    if(p_FileTo.exists()){
        n_SizeFileTo = p_FileTo.size();  // размер файла в байтах
        p_FileTo.close();
    }
        if(m_copyCancel==0 && n_SizeFileBegin == n_SizeFileTo && n_SizeFileTo){
        emit copyStatus(CopyisSuccessful);
        nNumbBytesRead = nNumbBytesWrite = 0;
    }
}
