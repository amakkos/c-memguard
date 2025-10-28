/*
 * $Header: /opt/cvsroot/projects/shark/packages/megamouth-atlantis/common/memguard.cc,v 1.14 2016/02/29 12:15:40 makki Exp $
 *
 * MemGuard source file
 * replaces malloc/calloc/realloc/free functions to debug memory allocation
 *
 * $Log: memguard.cc,v $
 * Revision 1.14  2016/02/29 12:15:40  makki
 * Cleanup error messages
 *
 * Revision 1.13  2016/02/25 13:06:50  makki
 * Fixes for MBT crash and DDM type
 *
 * Revision 1.12  2014/12/16 11:20:41  makki
 * Restored the feature to compile on linux (CentOS 6.5)
 *
 * Revision 1.11  2011/01/04 19:55:08  makki
 * fix for memory bugs reported by glibc
 *
 * Revision 1.10  2010/01/25 14:09:08  makki
 * Added safe strlen() and fclose()
 *
 * Revision 1.9  2010/01/20 10:15:16  makki
 * crash fix : handle strlen(null)
 *
 * Revision 1.8  2009/04/01 16:35:32  makki
 * Minore upgrade for memguard
 *
 * Revision 1.7  2009/03/27 18:57:36  makki
 * fix memory leaking in scheduler.cc:SchedStartTC(466)
 *
 * Revision 1.6  2009/03/25 20:53:16  makki
 * Improved MemGuard debug feature
 *
 * Revision 1.5  2008/12/05 10:44:32  makki
 * improved debug
 *
 * Revision 1.4  2008/12/01 14:21:11  makki
 * Fixing realloc in memeguard: do not call realloc for 1st allocation and to degrease the block
 *
 * Revision 1.3  2008/10/02 19:33:56  makki
 * re-designed memguard feature
 *
 * Revision 1.2  2008/09/30 16:16:58  makki
 * restored linux compatibility
 *
 * Revision 1.1  2008/05/07 07:26:38  makki
 * Initial release
 *
 *
 */

// *INDENT-OFF*
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#define _MEMGUARD_C_
#include "memguard.h"
#include <assert.h>
#include <time.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>


#define MG_FLAG_STRUCT
#define REP_WARNING 0	/* debug level of messge */
#define REP_ERROR   1	/* debug level of messge */
const char *Level[] = {"warning", "error"};

#define MEM_BLOCK_ATOM 128	/* atomic memory block size - save CPU time to allocate more than req.	*/
				/* possibly we can skip next (or a few) realloc call(s)			*/

/*
  TODO - improve statistics
static unsigned long mg_MaxBlocks = 0;
static unsigned long mg_MaxAllocation = 0;
static unsigned long mg_CurBlocks = 0;
static unsigned long mg_CurAllocation = 0;
static unsigned long mg_Allocations = 0;
static unsigned long mg_Frees = 0;
static unsigned long mg_TotalAllocation = 0;
static unsigned long mg_AllocationLimit = 0xffffffff;
*/

#ifdef MG_FLAG_STRUCT
unsigned long MG_CHECK_PATT=0xdeadbeef;	/* pattern to be saved at the end of allocated memory - checked at release */

struct Header {
  char            File[32];	/* The source file of the allocator */
  char            Func[32];     /* The source function of the allocator */
  unsigned long   Line;		/* The source line of the allocator */
  void           *ptr;		/* the allocated pointer */
  void           *chk;		/* the check pattern pointer allocated at the end */
  size_t          size;		/* the allocated size */
  time_t	  crte;		/* timestamp when the memory was allocated */
  struct Header  *Prev;		/* Previous link */
  struct Header  *Next;		/* Next link */
};

struct Header *mg_start=NULL;		/* chained list first record */
struct Header *mg_end=NULL;		/* chained list last record */
static unsigned long mg_NrBlocks = 0;	/* nr of current allocated blocks */
static unsigned long mg_TotMem = 0;	/* size of current allocated memory */
#endif



/* ============================================================================================================*/
/* ========================= chained list handlers ============================================================*/
/* ============================================================================================================*/

/******************************************************************
 *  get_patt_ptr - return the check pattern pointer of block
 ******************************************************************/
int *get_patt_ptr(void *ptr,size_t size)
{
  return ((int *)((size_t)ptr+size));
}

/******************************************************************
 *  node_print - flush out a node
 ******************************************************************/
void node_print(struct Header *hdr, FILE *tty)
{
  char timestr[32],*charptr;
  int i,grd;
  int *patt=get_patt_ptr(hdr->ptr,hdr->size);
  if (*patt == MG_CHECK_PATT)
    grd=0;
  else
    grd=1;

  strftime(timestr,sizeof(timestr),"%Y-%m-%d %H:%M:%S",localtime(&(hdr->crte)));
  fprintf(tty,"%s %5ld   %d %s:%s(%lu)",timestr,hdr->size, grd, hdr->File,hdr->Func,hdr->Line);
  fprintf(tty," '");
  /* write out all 'String' characters until strlen */
  for(i=0;i<hdr->size && i<50 && *((char *)((size_t)hdr->ptr+i))!=0;i++)
  {
    charptr=(char *)((size_t)hdr->ptr+i);
    if((*charptr >= 'a' && *charptr <= 'z' ) ||
       (*charptr >= 'A' && *charptr <= 'Z' ) ||
       (*charptr >= '0' && *charptr <= '9' ) ||
       *charptr == ' ' || 
       *charptr == ':' || 
       *charptr == '/' || 
       *charptr == '-' || 
       *charptr == '+' || 
       *charptr == '_' || 
       *charptr == '=' || 
       *charptr == '@' || 
       *charptr == ',' )
      fprintf(tty,"%c",*charptr);
    else
      fprintf(tty,".");
  }
  /* show cont'd when container is greater and string terminator not catched */
  if(i<hdr->size && *((char *)((size_t)hdr->ptr+i))!=0)
    fprintf(tty,"...");
  fprintf(tty,"'\n");
}

/******************************************************************
 *  chain_print - flush out the current list
 ******************************************************************/
void chain_print(FILE *tty)
{
  fprintf(tty, "\n");
  fprintf(tty, "MemGuard summary\n");
  fprintf(tty, "Blocks: %lu\n",mg_NrBlocks);
  fprintf(tty, "Memory: %lu\n",mg_TotMem);
  fprintf(tty, "timestamp           size  chk file:function(line)      \n");
  fprintf(tty, "=================== ===== === =========================\n");

  struct Header *hdr=mg_start;
  while (hdr!=NULL)
  {
    node_print(hdr, tty);
    hdr=hdr->Next;
  }
}

/******************************************************************
 *  MGPrint - flush out the current list into stderr - by signal
 ******************************************************************/
void MGPrint(int sig)
{
#ifdef MEMGUARD
  chain_print(stderr);
  fflush(stderr);
#endif
}

/******************************************************************
 *  MGLog - write into the master log
 ******************************************************************/
void MGLog(int warning, const char *fmt, ...)
{
  char            timestr[32];
  time_t          now;
  char            filename[50];
  FILE           *fd;
  va_list         ap;

#ifdef LOGDIR
  sprintf(filename, "%s/memguard.log", LOGDIR);
  if (!(fd = fopen(filename, "a+")))
  {
    fprintf(stderr, "MGReport - Error: file %s could not be opened\n", filename);
    return;
  }
#else
  fd=stderr;
#endif

  now=time(NULL);
  strftime(timestr,sizeof(timestr),"%Y.%m.%d %T",localtime(&now));

  va_start(ap, fmt);
  fprintf(fd, "%s ", timestr);
  vfprintf(fd, fmt, ap);
#ifdef LOGDIR  // if not logging into a file, do not fclose stderr
  fflush(fd);
  fclose(fd);
#endif
  va_end(ap);
}


/******************************************************************
 *  chain_add - adds a chained list member at the end
 ******************************************************************/
void chain_add(const char *FUNC, const char *FILEN, unsigned long LINE, void *ptr, size_t size)
{  
  struct Header *hdr=NULL;
  int len;
  
  hdr=(struct Header *)calloc(1,sizeof(struct Header));
  assert(hdr);
  memset(hdr->File,0,sizeof(hdr->File));
  snprintf(hdr->File,sizeof(hdr->File), "%s", FILEN);
  memset(hdr->Func,0,sizeof(hdr->Func));
  len=sizeof(hdr->Func);
  if(strstr(FUNC,"(")!=NULL && strstr(FUNC,"(")-FUNC < len)
    len=strstr(FUNC,"(")-FUNC+1;
  snprintf(hdr->Func,len, "%s", FUNC);
  hdr->Line=LINE;
  hdr->ptr=ptr;
  hdr->size=size;
  hdr->crte=time(NULL);
  
  if(mg_end!=NULL)
  {
    mg_end->Next=hdr;
    hdr->Next=NULL;
    hdr->Prev=mg_end;
  }
  mg_end=hdr;
  if(mg_start==NULL)
  {
    mg_start=hdr;
    hdr->Prev=NULL;
  }
}

/******************************************************************
 *  chain_serach_by_ptr - searches a chained list member by memory ptr
 ******************************************************************/
struct Header *chain_serach_by_ptr(void *ptr)
{
  struct Header *hdr;
  hdr=mg_start;
  while(hdr!=NULL && hdr->ptr!=ptr)
    hdr=hdr->Next;
  return hdr;
}

/******************************************************************
 *  chain_remove - removes a chained list member
 ******************************************************************/
void chain_remove(struct Header *hdr)
{
  if(hdr->Prev!=NULL)
    hdr->Prev->Next=hdr->Next;
  if(hdr->Next!=NULL)
    hdr->Next->Prev=hdr->Prev;

  if(mg_start==hdr)
    mg_start=hdr->Next;
  if(mg_end==hdr)
    mg_end=hdr->Prev;

  free(hdr);
}


/* ============================================================================================================*/
/* ========================= block register add and remove handlers ===========================================*/
/* ============================================================================================================*/

/******************************************************************
 *
 *  LogAlloc - Manage memory allocation
 *
 ******************************************************************/

void LogAlloc(const char *method, const char *FUNC, const char *FILEN, unsigned long LINE, void *ptr, size_t size)
{
  if(ptr==NULL)
  {
    fprintf(stderr,"Memguard - Memory allocation failed at %s in %s:%s(%ld)\n",method,FILEN, FUNC, LINE);
    fflush(stderr);
    exit(1);
  }
#ifdef MG_FLAG_FILES
  FILE *fd;
  char fname[62];
  char dname[32];

  sprintf(dname, "/tmp/MemGuard-%d", getpid());
  mkdir(dname, 0777);

  sprintf(fname, "%s/%x", dname, ptr);
  fd = fopen(fname, "w");
  if (fd != NULL)
  {
    fprintf(fd, "%s(%lu) %lu\n", FILEN, LINE, size);
    fclose(fd);
  }
#endif

#ifdef MG_FLAG_STRUCT
  mg_NrBlocks++;
  mg_TotMem+=(unsigned long)size;
  int *patt=get_patt_ptr(ptr,size);
  *patt = MG_CHECK_PATT;
  chain_add(FUNC, FILEN, LINE, ptr, size);
#endif
}

/******************************************************************
 *  CheckPointer - check pattern of block
 ******************************************************************/
void CheckPointer(const char *method, const char *FILEN, const char *FUNC, unsigned long LINE, void *ptr)
{
  struct Header *hdr;
  hdr=chain_serach_by_ptr(ptr);
  if(hdr!=NULL)
  {
    int *patt=get_patt_ptr(hdr->ptr,hdr->size);
    if (*patt != MG_CHECK_PATT)
    {
      MGLog(REP_ERROR,"MemGuard: WARNING - overflow detected at %s in %s:%s(%d) in block allocated by %s:%s(%d)\n", method, FILEN, FUNC, LINE,hdr->File,hdr->Func,hdr->Line);
    }
  }
// Unfortunatelly that one reports all writings to the staticaly allocated char[] arrays
//  else
//  {
//    MGLog(REP_WARNING,"MemGuard: WARNING - writing into an unallocated block in %s:%s(%d)\n", FILEN, FUNC, LINE);
//  }
}

/******************************************************************
 *
 *  LogFree - Manage memory release
 *
 ******************************************************************/
void LogFree(const char *method, const char *FILEN, const char *FUNC, unsigned long LINE, void *ptr)
{
#ifdef MG_FLAG_FILES
  char fname[64];

  sprintf(fname, "/tmp/MemGuard-%d/%x", getpid(), ptr);
  unlink(fname);
#endif
#ifdef MG_FLAG_STRUCT
  int grd;
  struct Header *hdr;
  hdr=chain_serach_by_ptr(ptr);
  if(hdr!=NULL)
  {
    mg_NrBlocks--;
    mg_TotMem-=(unsigned long)(hdr->size);
    CheckPointer(method, FILEN, FUNC, LINE, ptr);
    chain_remove(hdr);
  }
#endif
}


/* ============================================================================================================*/
/* ========================= replacement function for memory allocation and release ===========================*/
/* ============================================================================================================*/


/******************************************************************
 *  MGMalloc - replacement of void *malloc (size_t Size)
 ******************************************************************/
void *MGMalloc(const char *FILEN, const char *FUNC, unsigned long LINE, size_t size)
{
  void *pointer;
  size=((int(size / 512) +1) * 512);

  pointer = malloc(size+sizeof(MG_CHECK_PATT));
  LogAlloc("malloc",FUNC,FILEN, LINE, pointer, size);
  return pointer;
}

/******************************************************************
 *  MGCalloc - replacement of void *calloc (size_t NumberOfElements,size_t ElementSize)
 ******************************************************************/
void *MGCalloc(const char *FILEN, const char *FUNC, unsigned long LINE, size_t NumberOfElements, size_t ElementSize)
{
  void *pointer;
  int size=NumberOfElements * ElementSize;
//  size=((int(size / MEM_BLOCK_ATOM) +1) * MEM_BLOCK_ATOM);

  pointer = malloc(size + sizeof(MG_CHECK_PATT));
  memset(pointer,0,size);
  LogAlloc("calloc", FUNC,FILEN, LINE, pointer, size);
  return pointer;
}

/******************************************************************
 *  MGRealloc - replacement of void *realloc (void *Pointer,size_t Size)
 ******************************************************************/
void *MGRealloc(const char *FILEN, const char *FUNC, unsigned long LINE, void *ptr, size_t size)
{
  void *pointer;
  struct Header *hdr;

  if(ptr==NULL)
  {
    MGLog(REP_WARNING, "Memguard - realloc called against NULL in %s:%s(%d)\n",FILEN, FUNC, LINE);
    pointer = malloc(size + sizeof(MG_CHECK_PATT));
    LogAlloc("realloc", FUNC, FILEN, LINE, pointer, size);
    return pointer;
  }
  else
  {
    CheckPointer("realloc", FILEN, FUNC, LINE, ptr);
//    LogFree("realloc", FILEN, FUNC, LINE, ptr);

    hdr=chain_serach_by_ptr(ptr);
    if(hdr!=NULL)
      if(hdr->size>=size)
        return hdr->ptr;

    size=((int(size / MEM_BLOCK_ATOM) +1) * MEM_BLOCK_ATOM);
//    if(hdr!=NULL)
//      fprintf(stderr,"Memguard - realloc oldsize:%lu newsize:%lu\n",hdr->size,size);

    pointer = realloc(ptr, size + sizeof(MG_CHECK_PATT));
    if(pointer==NULL)
    {
      fprintf(stderr,"Memguard - Memory allocation failed at realloc in %s:%s(%ld)\n",FILEN, FUNC, LINE);
      if(hdr!=NULL)
        fprintf(stderr,"           Originally allocated %lu bytes in %s:%s(%ld)\n",hdr->size,hdr->File,hdr->Func,hdr->Line);
      fflush(stderr);
      MGPrint(15);
      exit(1);
    }

    if(hdr!=NULL)
    {
      hdr->size=size;
      if(ptr!=pointer)
        hdr->ptr=pointer;
      int *patt=get_patt_ptr(hdr->ptr,hdr->size);
      *patt = MG_CHECK_PATT;
    }
    else
      LogAlloc("realloc", FUNC, FILEN, LINE, pointer, size);
    return pointer;
  }
}

/******************************************************************
 *  MGFree - replacement of void free (void *Pointer)
 ******************************************************************/
void MGFree(const char *FILEN, const char *FUNC, unsigned long LINE, void *ptr)
{
  LogFree("free", FILEN, FUNC, LINE, ptr);
  free(ptr);
}
/******************************************************************
 *  GFree - a GNU compatible replacement of void free (void *Pointer)
 ******************************************************************/
void GFree(void *ptr)
{
  if(ptr!=NULL)
    free(ptr);
}


/******************************************************************
 *  MGSprintf - replacement of int snprintf (void *Pointer)
 ******************************************************************/
int MGSprintf(const char *FILEN, const char *FUNC, unsigned long LINE, char *target, const char *fmt...)
{
  va_list ap;

  if(target==NULL)
    return(0);
  va_start(ap, fmt);
  int rc=vsprintf(target, fmt, ap);
  va_end(ap);
  CheckPointer("sprintf", FILEN, FUNC, LINE, target);
  return(rc);
}

/******************************************************************
 *  MGSnprintf - replacement of int snprintf (void *Pointer)
 ******************************************************************/
int MGSnprintf(const char *FILEN, const char *FUNC, unsigned long LINE, char *target, int len, const char *fmt...)
{
  va_list ap;
  
  if(target==NULL)
    return(0);
  va_start(ap, fmt);
  int rc=vsnprintf(target, len, fmt, ap);
  va_end(ap);
  
  CheckPointer("snprintf", FILEN, FUNC, LINE, target);
  return(rc);
}


/******************************************************************
 *  MGSnprintf - replacement of int snprintf (void *Pointer)
 ******************************************************************/
char *MGstrcpy(const char *FILEN, const char *FUNC, unsigned long LINE, char *dest, const char *src)
{
  char *pointer = strcpy(dest,src);
  CheckPointer("strcpy", FILEN, FUNC, LINE, dest);
  return(pointer);
}

/******************************************************************
 *  MGSnprintf - replacement of int snprintf (void *Pointer)
 ******************************************************************/
char *MGstrncpy(const char *FILEN, const char *FUNC, unsigned long LINE, char *dest, const char *src, size_t n)
{
  char *pointer = strncpy(dest,src,n);
  CheckPointer("strncpy", FILEN, FUNC, LINE, dest);
  return(pointer);
}


/******************************************************************
 *  MGSnprintf - replacement of int snprintf (void *Pointer)
 ******************************************************************/
void *MGmemmove(const char *FILEN, const char *FUNC, unsigned long LINE, void *dest, const void *src, size_t n)
{
  void *pointer = memmove(dest,src,n);
  CheckPointer("memmove", FILEN, FUNC, LINE, dest);
  return(pointer);
}

/******************************************************************
 *  MGSnprintf - replacement of int snprintf (void *Pointer)
 ******************************************************************/
void *MGmemcpy(const char *FILEN, const char *FUNC, unsigned long LINE, void *dest, const void *src, size_t n)
{
  void *pointer = memcpy(dest,src,n);
  CheckPointer("memcpy", FILEN, FUNC, LINE, dest);
  return(pointer);
}

/******************************************************************
 *  MGSnprintf - replacement of int snprintf (void *Pointer)
 ******************************************************************/
void *MGmemset(const char *FILEN, const char *FUNC, unsigned long LINE, void *s, int c, size_t n)
{
  void *pointer = memset(s,c,n);
  CheckPointer("memset", FILEN, FUNC, LINE, s);
  return(pointer);
}

#ifdef _LINUX_
/******************************************************************
 *  MGstrlen - replacement of 'size_t strlen(const char *s)'
 ******************************************************************/
size_t MGstrlen(const char *FILEN, unsigned long LINE, const char *n)
{
  if(n==NULL)
  {
    MGLog(REP_WARNING, "Memguard - WARNING: strlen(null) was called from %s line %d\n",FILEN, LINE);
    return 0;
  }
  else
    return strlen(n);
}

/******************************************************************
 *  MGfclose - replacement of 'int fclose(FILE *fp)'
 ******************************************************************/
int MGfclose(const char *FLE, unsigned long LINE, FILE *n)
{
  if(n==NULL)
  {
    MGLog(REP_WARNING, "Memguard - WARNING: fclose(null) was called from %s line %d\n",FLE, LINE);  
    return 0;
  }
  else
    return fclose(n);
}
#endif /* LINUX */

// *INDENT-ON*
