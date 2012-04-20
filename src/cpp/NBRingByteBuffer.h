#ifndef NBRINGBYTEBUFFER_H
#define NBRINGBYTEBUFFER_H

#include <new>
#include <algorithm>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

#include <sched.h>
#include <pthread.h>

#define NBLIKELY(expr)    __builtin_expect(!!(expr), true)
#define NBUNLIKELY(expr)  __builtin_expect(!!(expr), false)

inline unsigned long round2(unsigned long x) {
	unsigned long tmp = 1;
	while (tmp < x)
		tmp <<= 1;
	return tmp;
}

class NBRingByteBuffer {
public:
	NBRingByteBuffer(size_t size);
	~NBRingByteBuffer();

	void* alloc_write(size_t size, unsigned long* transaction);
	void commit_write(unsigned long transaction);

	void* fetch_read(size_t* size, unsigned long* transaction);
	void commit_read(unsigned long transaction);

	void flush();
	void resize(size_t size);

private:
	char* buffer;
	unsigned long mask;

	unsigned long w_commited;
	unsigned long w_current;
	unsigned long r_commited;
	unsigned long r_current;

	pthread_mutex_t mutex;
	pthread_cond_t nonempty;
	pthread_cond_t nonfull;
};

inline NBRingByteBuffer::NBRingByteBuffer(size_t size) : w_commited(0), w_current(0), r_commited(0), r_current(0)
{
	char path[] = "/dev/shm/ring-buffer-XXXXXX";
	int fd;
	void *address;

	size_t ps = getpagesize();
	// Buffer should be aligned to page size or the next power of 2 of the requested size
	size = std::max(ps, round2(size));
	this->mask = size - 1;

	if ((fd = mkstemp(path)) < 0)
		throw std::bad_alloc();

	unlink(path);

	if(ftruncate(fd, size))
		throw std::bad_alloc();

	if ((this->buffer = (char*) mmap(NULL, size << 1, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0)) == MAP_FAILED)
		throw std::bad_alloc();

	address = mmap(this->buffer, size, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, fd, 0);

	if (address != this->buffer)
		throw std::bad_alloc();

	address = mmap(this->buffer + size, size, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, fd, 0);

	if (address != this->buffer + size)
		throw std::bad_alloc();

	close(fd);

	// Commit our buffer
	for (unsigned int i = 0; i < size; i += ps)
		*(int *)(this->buffer + i) = 0;

	pthread_mutex_init(&this->mutex, NULL);
	pthread_cond_init(&this->nonempty, NULL);
	pthread_cond_init(&this->nonfull, NULL);
}

inline NBRingByteBuffer::~NBRingByteBuffer()
{
	munmap(this->buffer, (this->mask + 1) << 1);

	pthread_mutex_destroy(&this->mutex);
	pthread_cond_destroy(&this->nonempty);
	pthread_cond_destroy(&this->nonfull);
}

inline void* NBRingByteBuffer::alloc_write(size_t size, unsigned long* transaction)
{
	// Check size of the block to write
	if (size > this->mask + 1 + sizeof(size_t)) {
		*transaction = 0;
		return 0;
	}

	*transaction = __sync_fetch_and_add(&this->w_current, sizeof(size_t) + size);
	while (NBUNLIKELY((*transaction + sizeof(size_t) + size - this->r_commited) & ~this->mask)) {
		pthread_mutex_lock(&this->mutex);
		if ((*transaction + sizeof(size_t) + size - this->r_commited) & ~this->mask)
			pthread_cond_wait(&this->nonfull, &this->mutex);
		pthread_mutex_unlock(&this->mutex);
	}
	*(size_t*)(this->buffer + (*transaction & this->mask)) = size;
	return this->buffer + ((*transaction + sizeof(size_t)) & this->mask);
}

inline void NBRingByteBuffer::commit_write(unsigned long transaction)
{
	while (NBUNLIKELY(!__sync_bool_compare_and_swap(&this->w_commited, transaction, transaction + sizeof(size_t) + *(size_t*)(this->buffer + (transaction & this->mask)))))
		sched_yield();
//	pthread_mutex_lock(&this->mutex);
	pthread_cond_broadcast(&this->nonempty);
//	pthread_mutex_unlock(&this->mutex);
}

inline void* NBRingByteBuffer::fetch_read(size_t* size, unsigned long* transaction)
{
	*transaction = this->r_current;
	while (NBUNLIKELY(*transaction == this->w_commited)) {
		pthread_mutex_lock(&this->mutex);
		if (*transaction == this->w_commited)
			pthread_cond_wait(&this->nonempty, &this->mutex);
		pthread_mutex_unlock(&this->mutex);
	}

	while(NBUNLIKELY(!__sync_bool_compare_and_swap(&this->r_current, *transaction, *transaction + sizeof(size_t) + *(size_t*)(this->buffer + (*transaction & this->mask))))) {
		sched_yield();
		*transaction = this->r_current;

		while (NBUNLIKELY(*transaction == this->w_commited)) {
			pthread_mutex_lock(&this->mutex);
			if (*transaction == this->w_commited)
				pthread_cond_wait(&this->nonempty, &this->mutex);
			pthread_mutex_unlock(&this->mutex);
		}
	}

	*size = *(size_t*)(this->buffer + (*transaction & this->mask));
	return this->buffer + ((*transaction + sizeof(size_t)) & this->mask);
}

inline void NBRingByteBuffer::commit_read(unsigned long transaction)
{
	while (NBUNLIKELY(!__sync_bool_compare_and_swap(&this->r_commited, transaction, transaction + sizeof(size_t) + *(size_t*)(this->buffer + (transaction & this->mask)))))
		sched_yield();
	pthread_mutex_lock(&this->mutex);
	pthread_cond_broadcast(&this->nonfull);
	pthread_mutex_unlock(&this->mutex);
}

#ifdef TEST_MAIN

#include <QThread>

#define BUF_SIZE 128
#define MAX_SIZE 50

NBRingByteBuffer* rb = new NBRingByteBuffer(BUF_SIZE * 1024 * 1024);

unsigned long written_items = 0;
unsigned long read_items = 0;

unsigned long written_bytes = 0;

class Producer : public QThread {
protected:
	virtual void run()
	{
		unsigned long tx, csum, val, *buf;
		int i, j;

		while (true) {
			i = qrand() % MAX_SIZE + MAX_SIZE;
			buf = (unsigned long*)rb->alloc_write(i * sizeof(unsigned long), &tx);
			__sync_fetch_and_add(&written_bytes, i * sizeof(unsigned long));
			csum = 0;
			for (j = i - 1; j; j--) {
				val = ((unsigned long)qrand()) << 32 | qrand();
				csum ^= val;
				*buf++ = val;
			}
			*buf = csum;
			rb->commit_write(tx);
			__sync_fetch_and_add(&written_items, 1);
		}
	}
};

class Consumer : public QThread {
protected:
	virtual void run()
	{
		unsigned long tx, csum, *buf;
		int i, j;
		size_t size;

		while (true) {
			buf = (unsigned long*)rb->fetch_read(&size, &tx);
			csum = 0;
			i = size / sizeof(unsigned long);
			for (j = i; j; j--)
				csum ^= *buf++;
			rb->commit_read(tx);
			if (csum != 0)
				printf("MISMATCH! %lu\n", csum);
			__sync_fetch_and_add(&read_items, 1);
		}
	}
};

int main()
{
	size_t last_written = 0, last_read = 0;
	unsigned long last_written_bytes = 0;

	for (int i = 0; i < 4; i++) {
		Producer* p = new Producer;
		Consumer* c = new Consumer;

		p->start();
		c->start();
	}

	while (true) {
		sleep(1);
		fprintf(stderr, "written: %ld (%lu MB), read: %ld\n", written_items - last_written, (written_bytes - last_written_bytes) >> 20, read_items - last_read);
		last_written = written_items;
		last_read = read_items;
		last_written_bytes = written_bytes;
	}

	delete rb;
	return 0;
}

#endif // TEST_MAIN

#endif // NBRINGBYTEBUFFER_H
