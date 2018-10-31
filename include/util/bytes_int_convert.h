#ifndef PRO_SERVICE_BYTS_INT_CONVERT_H
#define PRO_SERVICE_BYTS_INT_CONVERT_H

unsigned int bytes_to_int(const char *buf);
void int_to_bytes(char *buf,unsigned int val);
void int_to_ip(unsigned int val ,u8 *str,int size);

#endif //PRO_SERVICE_BYTS_INT_CONVERT_H
