#ifndef MD5_H
#define MD5_H

int Compute_string_md5(unsigned char *dest_str, unsigned int dest_len, char *md5_str);
int Compute_file_key_md5(const char *file_path, char *md5_str);
int Compute_file_md5(const char *file_path, char *md5_str);
int write_file_key_md5(const char *file_path);
bool check_file_key_md5(const char *file_path);
unsigned int get_file_size(const char *filename);

#endif

