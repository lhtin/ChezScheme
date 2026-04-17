/* ctype.h -- Character classification for bare-metal RV64G */

#ifndef _BAREMETAL_CTYPE_H
#define _BAREMETAL_CTYPE_H

int isalpha(int c);
int isdigit(int c);
int isalnum(int c);
int isspace(int c);
int isupper(int c);
int islower(int c);
int isprint(int c);
int ispunct(int c);
int iscntrl(int c);
int isxdigit(int c);
int isgraph(int c);
int isascii(int c);
int isblank(int c);
int toupper(int c);
int tolower(int c);
int toascii(int c);

/* Wide char classification stubs needed by locale-aware code */
int _toupper(int c);
int _tolower(int c);

#endif /* _BAREMETAL_CTYPE_H */
