
#ifndef WHEELS_BASE64_H
#define WHEELS_BASE64_H
int base64_decode(char *b64message, char **buffer);
int base64_encode(const char* message, char** buffer);
#endif //WHEELS_BASE64_H
