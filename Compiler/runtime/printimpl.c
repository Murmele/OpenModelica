/*
 * This file is part of OpenModelica.
 *
 * Copyright (c) 1998-2010, Linköpings University,
 * Department of Computer and Information Science,
 * SE-58183 Linköping, Sweden.
 *
 * All rights reserved.
 *
 * THIS PROGRAM IS PROVIDED UNDER THE TERMS OF THIS OSMC PUBLIC
 * LICENSE (OSMC-PL). ANY USE, REPRODUCTION OR DISTRIBUTION OF
 * THIS PROGRAM CONSTITUTES RECIPIENT'S ACCEPTANCE OF THE OSMC
 * PUBLIC LICENSE.
 *
 * The OpenModelica software and the Open Source Modelica
 * Consortium (OSMC) Public License (OSMC-PL) are obtained
 * from Linköpings University, either from the above address,
 * from the URL: http://www.ida.liu.se/projects/OpenModelica
 * and in the OpenModelica distribution.
 *
 * This program is distributed  WITHOUT ANY WARRANTY; without
 * even the implied warranty of  MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE, EXCEPT AS EXPRESSLY SET FORTH
 * IN THE BY RECIPIENT SELECTED SUBSIDIARY LICENSE CONDITIONS
 * OF OSMC-PL.
 *
 * See the full OSMC Public License conditions for more details.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "errorext.h"
#include "systemimpl.h"

/* adrpo: this is defined in errorext. (enabled with omc +showErrorMessages) */
extern int showErrorMessages;

#define GROWTH_FACTOR 1.4  /* According to some rumors of buffer growth */
#define INITIAL_BUFSIZE 4000 /* Seems reasonable */
#define MAXSAVEDBUFFERS 10   /* adrpo: added this so it compiles again! MathCore can change it later */

#define buf Print_var_buf
#define errorBuf Print_var_errorBuf
#define nfilled Print_var_nfilled
#define cursize Print_var_cursize
#define errorNfilled Print_var_errorNfilled
#define errorCursize Print_var_errorCursize

char *buf = NULL;
char *errorBuf = NULL;

int nfilled=0;
int cursize=0;

int errorNfilled=0;
int errorCursize=0;

 char** savedBuffers=0;
 long* savedCurSize;
 long* savedNfilled;

static int increase_buffer(void)
{

  char *new_buf;
  int new_size;
  if (cursize == 0) {
    new_buf = (char*)malloc(INITIAL_BUFSIZE*sizeof(char));
    if (new_buf == NULL) { return -1; }
    new_buf[0]='\0';
    cursize = INITIAL_BUFSIZE;
  } else {
    //fprintf(stderr,"increasing buffer from %d to %d \n",cursize,((int)(cursize * GROWTH_FACTOR)));
    new_buf = (char*)malloc((new_size =(int) (cursize * GROWTH_FACTOR))*sizeof(char));
    if (new_buf == NULL) { return -1; }
    memcpy(new_buf,buf,cursize);
    cursize = new_size;
  }
  if (buf) {
    free(buf);
  }
  buf = new_buf;
  return 0;
}

static int increase_buffer_fixed(int increase)
{
  char * new_buf;
  int new_size;

  if (cursize == 0) {
    new_buf = (char*)malloc(increase*sizeof(char));
    if (new_buf == NULL) { return -1; }
    new_buf[0]='\0';
    cursize = increase;
  } else {
  new_size = (int)(cursize+increase);
    //fprintf(stderr,"increasing buffer_FIXED_ from %d to %d \n",cursize,new_size);
    new_buf = (char*)malloc(new_size*sizeof(char));
    if (new_buf == NULL) { return -1; }
    //memcpy(new_buf,buf,cursize);
    cursize = new_size;
  }
  //printf("new buff size set to: %d\n",cursize);
  if (buf) {
    free(buf);
  }
  buf = new_buf;
  return 0;
}

static int error_increase_buffer(void)
{
  char * new_buf;
  int new_size;

  if (errorCursize == 0) {
    new_buf = (char*)malloc(INITIAL_BUFSIZE*sizeof(char));
    if (new_buf == NULL) { return -1; }
    new_buf[0]='\0';
    errorCursize = INITIAL_BUFSIZE;
  } else {
    new_buf = (char*)malloc((new_size =(int) (errorCursize * GROWTH_FACTOR))*sizeof(char));
    if (new_buf == NULL) { return -1; }
    memcpy(new_buf,errorBuf,errorCursize);
    errorCursize = new_size;
  }
  if (errorBuf) {
    free(errorBuf);
  }
  errorBuf = new_buf;
  return 0;
}

int print_error_buf_impl(const char *str)
{
  /*  printf("cursize: %d, nfilled %d, strlen: %d\n",cursize,nfilled,strlen(str));*/

  if (str == NULL) {
    return -1;
  }
  while (errorNfilled + strlen(str)+1 > errorCursize) {
    if (error_increase_buffer() != 0) {
      return -1;
    }
    /* printf("increased -- cursize: %d, nfilled %d\n",cursize,nfilled);*/
  }

  sprintf((char*)(errorBuf+strlen(errorBuf)),"%s",str);
  errorNfilled=strlen(errorBuf);
  return 0;
}

static void PrintImpl__setBufSize(long newSize)
{
  if (newSize > 0) {
    printf(" setting init_size to: %ld\n",newSize);
    increase_buffer_fixed(newSize);
  }
}

static void PrintImpl__unSetBufSize(void)
{
  increase_buffer_fixed(INITIAL_BUFSIZE);
}

/* Returns 0 on success; 1 on failure */
static int PrintImpl__printErrorBuf(const char* str)
{
  if (showErrorMessages) /* adrpo: should we show error messages while they happen? */
  {
    fprintf(stderr, "%s", str);
    fflush(stderr);
  }

  if (print_error_buf_impl(str) != 0) {
    return 1;
  }

  return 0;
}

static void PrintImpl__clearErrorBuf(void)
{
  errorNfilled=0;
  if (errorBuf != 0) {
    /* adrpo 2008-12-15 free the error buffer as it might have got quite big meantime */
    free(errorBuf);
    errorBuf = NULL;
    errorCursize=0;
  }
}

/* returns NULL on failure */
static const char* PrintImpl__getErrorString(void)
{
  if (errorBuf == 0) {
    if(error_increase_buffer() != 0) {
      return NULL;
    }
  }
  return errorBuf;
}

/* returns 0 on success */
static int PrintImpl__printBuf(const char* str)
{
  long len = strlen(str);
  /* printf("cursize: %d, nfilled %d, strlen: %d\n",cursize,nfilled,strlen(str)); */

  while (nfilled + len + 1 > cursize) {
    if(increase_buffer()!= 0) {
      return 1;
    }
    /* printf("increased -- cursize: %d, nfilled %d\n",cursize,nfilled); */
  }

  /*
   * nfilled += sprintf((char*)(buf+nfilled),"%s",str);
   * replaced by (below), courtesy of Pavol Privitzer
   */
  memcpy(buf+nfilled, str, len);
  nfilled += len;
  buf[nfilled] = '\0';

  return 0;
}

static void PrintImpl__clearBuf(void)
{
  nfilled=0;
  if (buf != 0) {
    /* adrpo 2008-12-15 free the print buffer as it might have got quite big meantime */
    free(buf);
    buf = NULL;
    cursize = 0;
    increase_buffer(); /* Initialize it; the saved buffers cannot handle NULL */
  }
}

/* returns NULL on failure */
const char* PrintImpl__getString(void)
{
  if(buf == NULL || buf[0]=='\0' || cursize==0){
    return "";
  }
  if (buf == 0) {
    if (increase_buffer() != 0) {
      return NULL;
    }
  }
  return buf;
}

/* returns 0 on success */
static int PrintImpl__writeBuf(const char* filename)
{
#if defined(__MINGW32__) || defined(_MSC_VER)
  const char *fileOpenMode = "wt"; /* on Windows do translation so that \n becomes \r\n */
#else
  const char *fileOpenMode = "wb";  /* on Unixes don't bother, do it binary mode */
#endif
  FILE * file = NULL;
  /* check if we have something to write */
  /* open the file */
  /* adrpo: 2010-09-22 open the file in BINARY mode as otherwise \r\n becomes \r\r\n! */
  file = fopen(filename,fileOpenMode);
  if (file == NULL) {
    const char *c_tokens[1]={filename};
    c_add_message(21, /* WRITING_FILE_ERROR */
      ErrorType_scripting,
      ErrorLevel_error,
      gettext("Error writing to file %s."),
      c_tokens,
      1);
    return 1;
  }

  if (buf == NULL || buf[0]=='\0') {
    /* nothing to write to file, just close it and return ! */
    fclose(file);
    return 1;
  }

  /*  write 1 element of size nfilled to file and check for errors */
  if (1 != fwrite(buf, nfilled, 1,  file))
  {
    const char *c_tokens[1]={filename};
    c_add_message(21, /* WRITING_FILE_ERROR */
      ErrorType_scripting,
      ErrorLevel_error,
      gettext("Error writing to file %s."),
      c_tokens,
      1);
    fprintf(stderr, "Print.writeBuf: error writing to file: %s!\n", filename);
    fclose(file);
    return 1;
  }
  if (fflush(file) != 0)
  {
    fprintf(stderr, "Print.writeBuf: error flushing file: %s!\n", filename);
  }
  fclose(file);
  return 0;
}

static long PrintImpl__getBufLength(void)
{
  return nfilled;
}

/* returns 0 on success */
static int PrintImpl__printBufSpace(long nSpaces)
{
  if (nSpaces > 0) {
   while (nfilled + nSpaces + 1 > cursize) {
     if(increase_buffer()!= 0) {
       return 1;
     }
   }
   memset(buf+nfilled,' ',(size_t)nSpaces);
   nfilled += nSpaces;
   buf[nfilled] = '\0';
  }
  return 0;
}

/* returns 0 on success */
static int PrintImpl__printBufNewLine(void)
{
  while (nfilled + 1+1 > cursize) {
    if(increase_buffer()!= 0) {
      return 1;
    }
  }
  buf[nfilled++] = '\n';
  buf[nfilled] = '\0';

  return 0;
}

static int PrintImpl__hasBufNewLineAtEnd(void)
{
  return (nfilled > 0 && buf[nfilled-1] == '\n') ? 1 : 0;
}

static int PrintImpl__restoreBuf(long handle)
{
  if (handle < 0 || handle > MAXSAVEDBUFFERS-1) {
    fprintf(stderr,"Internal error, hanlde %d out of range. Should be in [%d,&d]\n",handle,0,MAXSAVEDBUFFERS-1);
    return 1;
  } else {
    if (buf) { free(buf);}
    buf = savedBuffers[handle];
    cursize = savedCurSize[handle];
    nfilled = savedNfilled[handle];
    savedBuffers[handle] = 0;
    savedCurSize[handle] = 0;
    savedNfilled[handle] = 0;
    if (buf == 0) { 
      fprintf(stderr,"Internal error, handle %d does not contain a valid buffer pointer\n",handle);
      return 1;
    }
    return 0;
  }
}

static long PrintImpl__saveAndClearBuf()
{
  long freeHandle,foundHandle=0;

  if (! savedBuffers) { 
    savedBuffers = (char**)malloc(MAXSAVEDBUFFERS*sizeof(char*));
    if (!savedBuffers) { 
      fprintf(stderr, "Internal error allocating savedBuffers in Print.saveAndClearBuf\n");
      return -1;
    }
    memset(savedBuffers,0,MAXSAVEDBUFFERS);
  }
  if (! savedCurSize) { 
    savedCurSize = (long*)malloc(MAXSAVEDBUFFERS*sizeof(long*));
    if (!savedCurSize) { 
      fprintf(stderr, "Internal error allocating savedCurSize in Print.saveAndClearBuf\n");
      return -1;
    }
    memset(savedCurSize,0,MAXSAVEDBUFFERS);
  }
  if (! savedNfilled) { 
    savedNfilled = (long*)malloc(MAXSAVEDBUFFERS*sizeof(long*));
    if (!savedNfilled) { 
      fprintf(stderr, "Internal error allocating savedNfilled in Print.saveAndClearBuf\n");
      return -1;
    }
    memset(savedNfilled,0,MAXSAVEDBUFFERS);
  }
  for (freeHandle=0; freeHandle< MAXSAVEDBUFFERS; freeHandle++) {
    if (savedBuffers[freeHandle]==0)
    {
      foundHandle = 1;
      break;
    }
  }
  if (!foundHandle) {
    fprintf(stderr,"Internal error, can not save more than %d buffers, increase MAXSAVEDBUFFERS in printimpl.c\n",MAXSAVEDBUFFERS);
    return -1;
  }
  savedBuffers[freeHandle] = buf;
  savedCurSize[freeHandle] = cursize;
  savedNfilled[freeHandle] = nfilled;
  buf = (char*)malloc(INITIAL_BUFSIZE*sizeof(char));  
  nfilled=0;
  cursize=0;
  return freeHandle;
}
