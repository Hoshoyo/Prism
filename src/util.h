#pragma once
#include <ho_system.h>

#define FLAG(A) (1 << A)
#define MAX(X, Y) ((X > Y) ? (X) : (Y))
#define MIN(X, Y) ((X < Y) ? (X) : (Y))

#define MAKE_STRING(X) {-1, sizeof(X) - 1, X}

#define ARRAY_COUNT(X) (sizeof(X) / sizeof((X)[0]))

//
//	capacity -1 means immutable string
//
struct string
{
	s64 capacity;
	size_t length;

	char* data;

	string();
	string(s64 capac, size_t strlen, char* str);
	~string();
	void cat(string& r);
	void cat(char* str, size_t len);
	void cat(s64 num);
	
	bool is_mutable();
};

char* make_c_string(string& s);
string make_new_string(const char*);
string make_new_string(s64 capacity);
bool c_str_equal(const char* s1, const char* s2);
bool str_equal(const char* s1, int s1_len, const char* s2, int s2_len);
bool str_equal(const string& s1, const string& s2);
bool is_white_space(char str);

bool is_number(char c);
bool is_letter(char c);

s64 str_to_s64(char* text, int length);
s32 str_to_s32(char* text, int length);
r64 str_to_r64(char* text, int length);
r32 str_to_r32(char* text, int length);
u8  str_to_u8(char* text, int length);

void s32_to_str(s32 val, char* buffer);
void s64_to_str(s64 val, char* buffer);

u32 djb2_hash(u8 *str, int size);
u32 djb2_hash(u32 starting_hash, u8 *str, int size);
u32 djb2_hash(u32 hash1, u32 hash2);
u64 fnv_1_hash(char* s, u64 length);

s32 system_exit(s32 ret);