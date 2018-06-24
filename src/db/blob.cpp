// IMPORTANT NOTE:
//	Data inside `Blob` is stored in little endian byte order as
//	most servers run on X64 arch which is little endian so we avoid
//	the conversion overhead.

#include "blob.h"
#include "../def.h"
#include "../buffer_util.h"
#include "../system.h"
#include <string.h>

#define ROUND_TO_64(x) (((x) + 63) & ~63)

/*************************************

	Blob Implementation

*************************************/
Blob::Blob(size_t initial_capacity)
	: capacity_(initial_capacity), size_(0) {
	if(capacity_ == 0)
		UNREACHABLE();
	capacity_ = ROUND_TO_64(capacity_);
	data_ = (uint8*)sys_aligned_alloc(
		alignment, capacity_);
}
Blob::~Blob(void){
	sys_aligned_free(data_);
}

void Blob::check_capacity(size_t s){
	size_t req = size_ + s;
	if(req > capacity_){
		// calculate new capacity
		while(req > capacity_)
			capacity_ *= 2;
		capacity_ = ROUND_TO_64(capacity_);

		// reallocate data
		uint8 *ndata = (uint8*)sys_aligned_alloc(
			alignment, capacity_);
		memcpy(ndata, data_, size_);
		sys_aligned_free(data_);
		data_ = ndata;
	}
}

void Blob::add_u8(uint8 val){
	check_capacity(1);
	encode_u8(data_ + size_, val);
	size_ += 1;
}

void Blob::add_u16(uint16 val){
	check_capacity(2);
	encode_u16_le(data_ + size_, val);
	size_ += 2;
}

void Blob::add_u32(uint32 val){
	check_capacity(4);
	encode_u32_le(data_ + size_, val);
	size_ += 4;
}

void Blob::add_u64(uint64 val){
	check_capacity(8);
	encode_u64_le(data_ + size_, val);
	size_ += 8;
}

void Blob::add_f32(float val){
	check_capacity(4);
	encode_f32_le(data_ + size_, val);
	size_ += 4;
}

void Blob::add_f64(double val){
	check_capacity(8);
	encode_f64_le(data_ + size_, val);
	size_ += 8;
}

void Blob::add_data(uint8 *buf, size_t buflen){
	if(buflen == 0) return;
	check_capacity(buflen);
	memcpy(data_ + size_, buf, buflen);
	size_ += buflen;
}

uint8 Blob::get_u8(size_t readpos) const{
	return decode_u8(data_ + readpos);
}

uint16 Blob::get_u16(size_t readpos) const{
	return decode_u16_le(data_ + readpos);
}

uint32 Blob::get_u32(size_t readpos) const{
	return decode_u32_le(data_ + readpos);
}

uint64 Blob::get_u64(size_t readpos) const{
	return decode_u64_le(data_ + readpos);
}

float Blob::get_f32(size_t readpos) const{
	return decode_f32_le(data_ + readpos);
}

double Blob::get_f64(size_t readpos) const{
	return decode_f64_le(data_ + readpos);
}

void Blob::get_data(size_t readpos, uint8 *buf, size_t buflen) const{
	if((readpos + buflen) > size_)
		buflen -= size_ - readpos;
	memcpy(buf, (data_ + readpos), buflen);
}
