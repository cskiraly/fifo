/*
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX(a,b) ((a>b) ? a : b)
#define MIN(a,b) ((a>b) ? b : a)

size_t bufsize = 1024*1024;
size_t readsize = 1024 * 32;

void *buf, *rpos, *wpos;

static void cmdline_parse(int argc, char *argv[])
{
  int o;

  while ((o = getopt(argc, argv, "b:r:")) != -1) {
    switch(o) {
      case 'b':
        bufsize = atol(optarg);
      case 'r':
        readsize = atol(optarg);
        break;
      default:
        fprintf(stderr, "Error: unknown option %c\n", o);

        exit(-1);
    }
  }
}

int main(int argc, char *argv[])
{
  fd_set rfds, wfds;
  int reof = 0;
  int retval;

  cmdline_parse(argc, argv);

  buf = rpos = wpos = malloc(bufsize);
  if (!buf) {
    fprintf(stderr, "Error: can't allocate buffer of %lu bytes\n", bufsize);
    exit(EXIT_SUCCESS);
  }

  while(1) {

    /* Watch stdin (fd 0) to see when it has input. */
    FD_ZERO(&rfds);
    if (!reof && rpos != wpos-1 && rpos-bufsize != wpos-1 ) FD_SET(0, &rfds);

    /* Watch stdout (fd 1) to see when it can be written. */
    FD_ZERO(&wfds);
    if (rpos != wpos) FD_SET(1, &wfds);

    retval = select(2, &rfds, &wfds, NULL, NULL);

    if (retval == -1) {
      break;
    } else {
      if (FD_ISSET(0, &rfds)) {
        ssize_t size;
        size_t rmax;
        rmax = MIN(MIN(readsize, buf + bufsize - rpos), (wpos+bufsize-rpos-1) % bufsize);
        size = read(0, rpos, rmax);
//fprintf(stderr,"reading %lu bytes, read %ld bytes\n", rmax, size);
        if (size < 0) {
          break;
        } else if (size == 0)  {
          reof = 1;
        } else {
          rpos += size;
          if (rpos >= buf + bufsize) rpos -= bufsize;
        }
      }
      if (FD_ISSET(1, &wfds)) {
        ssize_t size;
        size_t wmax;
        wmax = (wpos > rpos) ? (buf + bufsize) - wpos  : rpos - wpos;
        size = write(1, wpos, wmax);
//fprintf(stderr,"writing %lu bytes, written %ld bytes\n", wmax, size);
        if (size < 0) {
          break;
        } else {
          wpos += size;
          if (wpos >= buf + bufsize) wpos -= bufsize;
        }
      }
    }
    if (reof && rpos == wpos) break;
  }
  free(buf);
  exit(EXIT_SUCCESS);
}