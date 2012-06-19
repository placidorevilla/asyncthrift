#ifndef TMEMFILE_H
#define TMEMFILE_H

#include <QIODevice>

class TMemFilePrivate;
class QFile;

class TMemFile : public QIODevice
{
	Q_OBJECT

public:
	explicit TMemFile(QObject *parent = 0);
	TMemFile(const QString& name, QObject *parent = 0);
	~TMemFile();

	QFile* file();

	bool open(OpenMode openMode);

	void close();
	qint64 size() const;
//	qint64 pos() const;
	bool seek(qint64 off);
//	bool atEnd() const;
	bool canReadLine() const;
	qint64 peek(char *data, qint64 maxSize);
	QByteArray peek(qint64 maxSize);

	bool sync();
	uchar* buffer();

protected:
	qint64 readData(char *data, qint64 maxlen);
	qint64 writeData(const char *data, qint64 len);

private slots:
	void _q_emitSignals();

private:
	Q_DISABLE_COPY(TMemFile)
	Q_DECLARE_PRIVATE(TMemFile)
	TMemFilePrivate* d_ptr;
};

#endif // TMEMFILE_H
