#include "TMemFile.h"

#include <QFile>
#include <QByteArray>

class TMemFilePrivate
{
	Q_DECLARE_PUBLIC(TMemFile)

public:
	TMemFilePrivate() {}
	explicit TMemFilePrivate(const QString& name) : file(name), buf(0), size(0), writtenSinceLastEmit(0), signalConnectionCount(0), signalsEmitted(false) {}
	~TMemFilePrivate() {}

	QFile file;
	uchar* buf;
	qint64 size;
	qint64 writtenSinceLastEmit;
	int signalConnectionCount;
	bool signalsEmitted;

	TMemFile* q_ptr;
};

void TMemFile::_q_emitSignals()
{
	Q_D(TMemFile);

	emit bytesWritten(d->writtenSinceLastEmit);
	d->writtenSinceLastEmit = 0;
	emit readyRead();
	d->signalsEmitted = false;
}

qint64 TMemFile::peek(char *data, qint64 maxSize)
{
	Q_D(TMemFile);

	qint64 readBytes = qMin(maxSize, d->size - pos());
	memcpy(data, d->buf + pos(), readBytes);
	return readBytes;
}

QByteArray TMemFile::peek(qint64 maxSize)
{
	Q_D(TMemFile);

	qint64 readBytes = qMin(maxSize, d->size - pos());
	return QByteArray((const char *)d->buf + pos(), readBytes);
}

TMemFile::TMemFile(QObject *parent) : QIODevice(parent), d_ptr(new TMemFilePrivate)
{
}

TMemFile::TMemFile(const QString& name, QObject *parent) : QIODevice(parent), d_ptr(new TMemFilePrivate(name))
{
}

TMemFile::~TMemFile()
{
}

QFile* TMemFile::file()
{
	Q_D(TMemFile);

	return &d->file;
}

bool TMemFile::open(OpenMode flags)
{
	Q_D(TMemFile);

	if ((flags & (Append | Truncate)) != 0)
		flags |= WriteOnly;
	if ((flags & (ReadOnly | WriteOnly)) == 0) {
		qWarning("TMemFile::open: Buffer access not specified");
		return false;
	}

	/* Append or truncate do not make sense in a mapped file */
	flags &= ~(Truncate | Append);

	/* To be able to mmap in WriteOnly, the file must be open for read as well as write */
	flags = flags | ReadOnly;

	if (!d->file.isOpen())
		d->file.open(flags);
	d->size = d->file.size();
	d->buf = d->file.map(0, d->size);
	if (d->buf == 0) {
		setErrorString(d->file.errorString());
		d->file.close();
		return false;
	}

	bool device_open = QIODevice::open(flags);
	if (!device_open) {
		d->file.unmap(d->buf);
		d->file.close();
	}

	return true;
}

void TMemFile::close()
{
	Q_D(TMemFile);

	if (!isOpen())
		return;

	d->file.unmap(d->buf);
	d->file.close();
	QIODevice::close();
}

#if 0
qint64 TMemFile::pos() const
{
	return QIODevice::pos();
}
#endif

qint64 TMemFile::size() const
{
	Q_D(const TMemFile);

	return d->size;
}

bool TMemFile::seek(qint64 pos)
{
	Q_D(TMemFile);

	if (pos > d->size || pos < 0) {
		qWarning("TMemFile::seek: Invalid pos: %d", int(pos));
		return false;
	}
	return QIODevice::seek(pos);
}

#if 0
bool TMemFile::atEnd() const
{
	return QIODevice::atEnd();
}
#endif

bool TMemFile::canReadLine() const
{
	Q_D(const TMemFile);

	if (!isOpen())
		return false;
	QByteArray ba = QByteArray::fromRawData((const char *)d->buf, d->size);

	return ba.indexOf('\n', int(pos())) != -1 || QIODevice::canReadLine();
}

qint64 TMemFile::readData(char *data, qint64 len)
{
	Q_D(TMemFile);

	if ((len = qMin(len, d->size - pos())) <= 0)
		return qint64(0);
	memcpy(data, d->buf + pos(), len);
	return len;
}

qint64 TMemFile::writeData(const char *data, qint64 len)
{
	Q_D(TMemFile);

	if (pos() + len > d->size) {
		qWarning("TMemFile::writeData: Cannot write beyond buffer");
		return -1;
	}

	memcpy(d->buf + pos(), (uchar *)data, int(len));

	d->writtenSinceLastEmit += len;
	if (d->signalConnectionCount && !d->signalsEmitted && !signalsBlocked()) {
		d->signalsEmitted = true;
		QMetaObject::invokeMethod(this, "_q_emitSignals", Qt::QueuedConnection);
	}
	return len;
}

bool TMemFile::sync()
{
	Q_D(const TMemFile);

	if (!isOpen() || !d->file.isOpen())
		return false;

	if (fdatasync(d->file.handle()) != 0) {
		setErrorString(qt_error_string(int(errno)));
		return false;
	}

	return true;
}

uchar* TMemFile::buffer()
{
	Q_D(TMemFile);
	return d->buf;
}

#if 0
int main()
{
	TMemFile t("test");

	if (!t.open(QIODevice::WriteOnly))
		printf("E: %s\n", qPrintable(t.errorString()));
	t.write("patata1", 7);
	t.write("patata2", 7);
	t.write("patata3", 7);
	t.close();

	t.open(QIODevice::ReadOnly);
	printf("t: '%.7s'\n", t.read(7).data());
	printf("t: '%.7s'\n", t.read(7).data());
	printf("t: '%.7s'\n", t.read(7).data());
	t.close();

	return 0;
}
#endif
