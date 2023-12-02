#include <QCoreApplication>
#include <QProcess>
#include <QStringList>
#include <QThread>
#include <QRegularExpression>

#include <iostream>

enum class ARGS_POS {
    MP3_FILE=1,
    NOICE=2,
    MIN_TIME=3
};

const QString defNoice     = "-50";
const QString defMinTime   = "0.80";

QStringList phrases = {
    "I am",
    "I'm",
    "You are",
    "You're",
    "He is",
    "He's",
    "She is",
    "She's",
    "It is",
    "It's",
    "We are",
    "We're",
    "They are",
    "They're",
};

QString outDir = "/home/m/Dokumenty/jula/english/to_be/";

int main(int argc, char *argv[])
{
    QString inputFile    = argv[(int)ARGS_POS::MP3_FILE];
    QString noice        = defNoice;
    QString silMinTime   = defMinTime;

    std::cout << "size of phrases:" << phrases.size() << std::endl;

    {
        const int idx = (int)ARGS_POS::NOICE;
        if( argc > idx ) {
            bool ok;
            QString(argv[idx]).toInt(&ok);
            if( ok ) {
                noice = argv[idx];
            }
        }
    }

    {
        const int idx = (int)ARGS_POS::MIN_TIME;
        if( argc >  idx ) {
            bool ok;
            QString(argv[idx]).toDouble(&ok);
            if( ok ) {
                silMinTime = argv[idx];
            }
        }
    }

    QProcess FFMPEGProcess;
    const QStringList params = {
        "-i",
        inputFile,
        "-af",
        QString("silencedetect=noise=%1dB:d=%2").arg(noice).arg(silMinTime,0,'f'),
        "-f",
        "null",
        "-"
    };
    FFMPEGProcess.setProgram("ffmpeg");
    FFMPEGProcess.setArguments(params);
    FFMPEGProcess.start();
    FFMPEGProcess.waitForFinished(-1);
    const QStringList lines = QString::fromUtf8(FFMPEGProcess.readAllStandardError()).split( QRegularExpression("[\\n\\r]+") , Qt::KeepEmptyParts );
    FFMPEGProcess.close();

    int state = 0;
    int nrPhrase = 0;
    double start = 0.0;
    double end   = 0.0;
    for( int i=0 ; i<lines.size() && nrPhrase<phrases.size() ; i++ ) {

        std::cout << i << " line:" << lines[i].toStdString() << std::endl;

        if( ! lines[i].startsWith("[silencedetect") ) {
            continue;
        }


        if( state == 0 ) {
            QRegularExpression rgx("^\\s*\\[silencedetect[^\\]]*\\].*silence_start.*:\\s*([\\.0-9]+).*$");
            QRegularExpressionMatch match = rgx.match(lines[i]);
            if( match.capturedStart() != 0 ) {
                abort();
            }
            if( rgx.captureCount() != 1 ) {
                abort();
            }
            bool ok;
            end = match.captured(1).toDouble(&ok);
            if( !ok || end < 0 ) {
                abort();
            }

            if( end - start > 0.1 )
            {
                const QString outFile = outDir + QString(phrases[nrPhrase]).toLower().replace(QRegularExpression("[^a-z0-9]")," ").trimmed().replace(QRegularExpression("\\s+"),"_").trimmed() + ".mp3";
                std::cout << QString("[%1/%2 - %3]").arg(nrPhrase).arg(phrases.size()).arg(phrases[nrPhrase]).toStdString() << " " << start << "-" << end << " file:" << outFile.toStdString() << "            >" << (end-start) << std::endl;
                nrPhrase++;



                //ffmpeg -y -i :IN_FILE -ss :BEGIN -to :END -c copy :OUT_FILE
                FFMPEGProcess.setProgram("ffmpeg");
                FFMPEGProcess.setArguments(
                QStringList{
                    "-y",
                    "-i",
                    inputFile,
                    "-ss",
                    QString::number(start,'f',2),
                    "-to",
                    QString::number(end,'f',2),
                    "-c",
                    "copy",
                    outFile
                });
                FFMPEGProcess.start();
                FFMPEGProcess.waitForFinished(-1);
                FFMPEGProcess.close();
            }

            state = 1;
        } else {
            QRegularExpression rgx("^\\s*\\[silencedetect[^\\]]*\\].*silence_end[^:]*:\\s*([\\.0-9]+).*$");
            QRegularExpressionMatch match = rgx.match(lines[i]);
//            if( match.indexIn( lines[i] ) != 0 ) {
//                abort();
//            }
//            if( match.capturedLength() != 1 ) {
//                abort();
//            }
            bool ok;
            start = match.captured(1).toDouble(&ok);
            if( !ok || start < 0 ) {
                abort();
            }
            state = 0;
        }
    }
    return 0;
}
