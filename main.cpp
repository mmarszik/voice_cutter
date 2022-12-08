#include <QCoreApplication>
#include <QProcess>
#include <QStringList>
#include <QThread>

#include <iostream>

enum class ARGS_POS {
    MP3_FILE=1,
    NOICE=2,
    MIN_TIME=3
};

const QString defNoice     = "-40";
const QString defMinTime   = "0.20";

QStringList phrases = {
    "to swim",
    "to go for a swim",
    "to have a swim ",
    "we had a lovely swimshall",
    "we have another swim?  ",
    "a good swim by Brown",
    "I swim",
    "I am swimming",
    "I'm swimming",
    "you swim",
    "you are swimming",
    "you're swimming",
    "he swims",
    "he is swimming",
    "he's swimming",
    "she swims",
    "she is swimming",
    "she's swimming",
    "it swims",
    "it is swimming",
    "it's swimming",
    "we swim",
    "we are swimming",
    "we're swimming",
    "they swim",
    "they are swimming",
    "they're swimming",
};

QString outDir = "/home/m/Dokumenty/tmp/out/";

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
    const QStringList lines = QString::fromUtf8(FFMPEGProcess.readAllStandardError()).split( QRegExp("[\\n\\r]+") , QString::KeepEmptyParts );
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
            QRegExp rgx("^\\s*\\[silencedetect[^\\]]*\\].*silence_start.*:\\s*([\\.0-9]+).*$",Qt::CaseInsensitive,QRegExp::RegExp2);
            if( rgx.indexIn( lines[i] ) != 0 ) {
                abort();
            }
            if( rgx.captureCount() != 1 ) {
                abort();
            }
            bool ok;
            end = rgx.cap(1).toDouble(&ok);
            if( !ok || end < 0 ) {
                abort();
            }

            if( end - start > 0.5 )
            {
                const QString outFile = outDir + QString(phrases[nrPhrase]).toLower().replace(QRegExp("[^a-z0-9]")," ").replace(QRegExp("\\s+"),"_").trimmed() + ".mp3";
                std::cout << nrPhrase << " " << start << "-" << end << " file:" << outFile.toStdString() << "            >" << (end-start) << std::endl;
                nrPhrase++;

                //ffmpeg -y -i :IN_FILE -ss :BEGIN -to :END -c copy :OUT_FILE
                FFMPEGProcess.setProgram("ffmpeg");
                FFMPEGProcess.setArguments(
                QStringList{
                    "-y",
                    "-i",
                    inputFile,
                    "-ss",
                    QString::number(start,0,'f'),
                    "-to",
                    QString::number(end,0,'f'),
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
            QRegExp rgx("^\\s*\\[silencedetect[^\\]]*\\].*silence_end[^:]*:\\s*([\\.0-9]+).*$",Qt::CaseInsensitive,QRegExp::RegExp2);
            if( rgx.indexIn( lines[i] ) != 0 ) {
                abort();
            }
            if( rgx.captureCount() != 1 ) {
                abort();
            }
            bool ok;
            start = rgx.cap(1).toDouble(&ok);
            if( !ok || start < 0 ) {
                abort();
            }
            state = 0;
        }
    }
    return 0;
}
