#ifndef _CLIB_DEP_H_
#define _CLIB_DEP_H_

int scanf(const char *fmt, ...);
int fscanf(FILE *fp, const char *fmt, ...);
int sscanf(const char *buf, const char *fmt, ...);
int atob(long unsigned int *vp, char *p, int base);
char *strsep(char *strp, const char *delims);
int strcspn (const char *p, const char *s);
int abs(int value);

#endif /* _CLIB_DEP_H_ */
