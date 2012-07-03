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
	virtual ~TMemFile();

	QFile* file();

	virtual bool open(OpenMode openMode);

	virtual void close();
	virtual qint64 size() const;
//	virtual qint64 pos() const;
	virtual bool seek(qint64 off);
//	virtual bool atEnd() const;
	virtual bool canReadLine() const;
	qint64 peek(char *data, qint64 maxSize);
	QByteArray peek(qint64 maxSize);

	bool sync();
	uchar* buffer();

protected:
	virtual qint64 readData(char *data, qint64 maxlen);
	virtual qint64 writeData(const char *data, qint64 len);

private slots:
	void _q_emitSignals();

private:
	Q_DISABLE_COPY(TMemFile)
	Q_DECLARE_PRIVATE(TMemFile)
	TMemFilePrivate* d_ptr;
};

#endif // TMEMFILE_H
