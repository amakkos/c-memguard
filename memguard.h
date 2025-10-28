/*
 * $Header: /opt/cvsroot/projects/shark/packages/megamouth-atlantis/common/memguard.h,v 1.9 2014/12/16 11:20:42 makki Exp $
 *
 * MemGuard header file
 * replaces malloc/calloc/realloc/free functions to debug memory allocation
 *
 * Include that into the C what you would like to debug. 
 * Make sure, memguard.o gets linked to your code.
 *
 * $Log: memguard.h,v $
 * Revision 1.9  2014/12/16 11:20:42  makki
 * Restored the feature to compile on linux (CentOS 6.5)
 *
 * Revision 1.8  2011/01/04 19:55:08  makki
 * fix for memory bugs reported by glibc
 *
 * Revision 1.7  2010/01/25 14:09:08  makki
 * Added safe strlen() and fclose()
 *
 * Revision 1.6  2010/01/20 10:15:16  makki
 * crash fix : handle strlen(null)
 *
 * Revision 1.5  2009/04/01 16:35:32  makki
 * Minore upgrade for memguard
 *
 * Revision 1.4  2009/03/25 20:53:16  makki
 * Improved MemGuard debug feature
 *
 * Revision 1.3  2008/12/05 10:44:33  makki
 * improved debug
 *
 * Revision 1.2  2008/10/02 19:33:56  makki
 * re-designed memguard feature
 *
 * Revision 1.1  2008/05/07 07:26:38  makki
 * Initial release
 *
 *
 */



void            MGPrint(int sig);


#ifdef MEMGUARD
void           *MGMalloc(const char *FLE, const char *FUNC, unsigned long LINE, size_t size);
void           *MGCalloc(const char *FLE, const char *FUNC, unsigned long LINE, size_t num, size_t size);
void           *MGRealloc(const char *FLE, const char *FUNC, unsigned long LINE, void *ptr, size_t size);
void            MGFree(const char *FLE, const char *FUNC, unsigned long LINE, void *ptr);
int             MGSprintf(const char *FLE, const char *FUNC, unsigned long LINE, char *target, const char *fmt ...);
int             MGSnprintf(const char *FLE, const char *FUNC, unsigned long LINE, char *target, int len, const char *fmt ...);

char           *MGstrcpy(const char *FLE, const char *FUNC, unsigned long LINE, char *dest, const char *src);
char           *MGstrncpy(const char *FLE, const char *FUNC, unsigned long LINE, char *dest, const char *src, size_t n);
void           *MGmemmove(const char *FLE, const char *FUNC, unsigned long LINE, void *dest, const void *src, size_t n);
void           *MGmemcpy(const char *FLE, const char *FUNC, unsigned long LINE, void *dest, const void *src, size_t n);
void           *MGmemset(const char *FLE, const char *FUNC, unsigned long LINE, void *s, int c, size_t n);

// void *MGmemchr(const char *FILE, const char *FUNC, unsigned long LINE, const void *s, int c, size_t n);
// void *MGmemrchr(const char *FILE, const char *FUNC, unsigned long LINE, const void *s, int c, size_t n);

#ifndef _MEMGUARD_C_    /* Only define the macros if we're NOT in memguard.c */
#define malloc(size)      MGMalloc(__FILE__, __FUNCTION__, __LINE__, size)
#define calloc(num,size)  MGCalloc(__FILE__, __FUNCTION__, __LINE__, num, size)
#define realloc(ptr,size) MGRealloc(__FILE__, __FUNCTION__, __LINE__, ptr, size)
#define free(ptr)         MGFree(__FILE__, __FUNCTION__, __LINE__, ptr)
#define sprintf(dst, fmt...)        MGSprintf(__FILE__, __FUNCTION__, __LINE__,  dst, ##fmt)
#define snprintf(dst, len, fmt...)  MGSnprintf(__FILE__, __FUNCTION__, __LINE__,  dst, len, ##fmt)

#define strcpy(dest,src)	MGstrcpy( __FILE__, __FUNCTION__, __LINE__,dest,src)
#define strncpy(dest,src,size)	MGstrncpy(__FILE__, __FUNCTION__, __LINE__,dest,src,size)
#define memmove(dest,src,size)	MGmemmove(__FILE__, __FUNCTION__, __LINE__,dest,src,size)
#define memcpy(dest,src,size)	MGmemcpy( __FILE__, __FUNCTION__, __LINE__,dest,src,size)
#define memset(s,c,n)		MGmemset( __FILE__, __FUNCTION__, __LINE__,s,c,n)
//#define memchr(s,c,n)         MGmemchr( __FILE__, __FUNCTION__, __LINE__,s,c,n)
//#define memrchr(s,c,n)        MGmemrchr(__FILE__, __FUNCTION__, __LINE__,s,c,n)

#endif /* _MEMGUARD_C_ */

#else /* MEMGUARD */
void            GFree(void *ptr);

#ifndef _MEMGUARD_C_
#define free(ptr)         GFree(ptr)
#endif /* _MEMGUARD_C_ */
#endif /* MEMGUARD */

/* always over define those functions */
#ifdef _LINUX_

size_t          MGstrlen(const char *FLE, unsigned long LINE, const char *n);
int             MGfclose(const char *FLE, unsigned long LINE, FILE *n);
void            MGPrint(int sig);

#ifndef _MEMGUARD_C_    /* Only define the macros if we're NOT in memguard.c */
#define strlen(n)	MGstrlen(__FILE__, __LINE__,n)
#define fclose(n)	MGfclose(__FILE__, __LINE__,n)
#endif /* _MEMGUARD_C_ */
#endif /* LINUX */

/***************************************
 * for reference : the original functions' parameter list 
 ***************************************
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
void *memchr(const void *s, int c, size_t n);
void *memrchr(const void *s, int c, size_t n);

char *strstr(const char *haystack, const char *needle);
char *strcasestr(const char *haystack, const char *needle);
size_t strlen(const char *s);
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
char *strtok(char *str, const char *delim);
char *strtok_r(char *str, const char *delim, char **saveptr);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
*/
