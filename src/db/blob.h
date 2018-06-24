#ifndef DB_BLOB_H_

#include "../def.h"

class Blob{
private:
	// making storage aligned might increase
	// performance in some cases
	static constexpr size_t alignment = 16;

	// storage control
	uint8 *data_;
	size_t capacity_;
	size_t size_;

	// helper function to grow buffer
	void check_capacity(size_t s);

public:
	// deleted operations
	Blob(void) = delete;
	Blob(const Blob&) = delete;
	Blob &operator=(const Blob&) = delete;

	Blob(size_t initial_capacity);
	~Blob(void);

	void *data(void) const;
	size_t capacity(void) const;
	size_t size(void) const;

	void add_u8(uint8 val);
	void add_u16(uint16 val);
	void add_u32(uint32 val);
	void add_u64(uint64 val);
	void add_f32(float val);
	void add_f64(double val);
	void add_data(uint8 *buf, size_t buflen);

	uint8 get_u8(size_t readpos) const;
	uint16 get_u16(size_t readpos) const;
	uint32 get_u32(size_t readpos) const;
	uint64 get_u64(size_t readpos) const;
	float get_f32(size_t readpos) const;
	double get_f64(size_t readpos) const;
	void get_data(size_t readpos, uint8 *buf, size_t buflen) const;
};

#endif //DB_BLOB_H_
