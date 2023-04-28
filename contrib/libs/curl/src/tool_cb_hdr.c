/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2022, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * SPDX-License-Identifier: curl
 *
 ***************************************************************************/
#include "tool_setup.h"

#include "strcase.h"

#define ENABLE_CURLX_PRINTF
/* use our own printf() functions */
#include "curlx.h"

#include "tool_cfgable.h"
#include "tool_doswin.h"
#include "tool_msgs.h"
#include "tool_cb_hdr.h"
#include "tool_cb_wrt.h"
#include "tool_operate.h"
#include "tool_libinfo.h"

#include "memdebug.h" /* keep this as LAST include */

static char *parse_filename(const char *ptr, size_t len);

#ifdef WIN32
#define BOLD
#define BOLDOFF
#else
#define BOLD "\x1b[1m"
/* Switch off bold by setting "all attributes off" since the explicit
   bold-off code (21) isn't supported everywhere - like in the mac
   Terminal. */
#define BOLDOFF "\x1b[0m"
/* OSC 8 hyperlink escape sequence */
#define LINK "\x1b]8;;"
#define LINKST "\x1b\\"
#define LINKOFF LINK LINKST
#endif

#ifdef LINK
static void write_linked_location(CURL *curl, const char *location,
    size_t loclen, FILE *stream);
#endif

/*
** callback for CURLOPT_HEADERFUNCTION
*/

size_t tool_header_cb(char *ptr, size_t size, size_t nmemb, void *userdata)
{
  struct per_transfer *per = userdata;
  struct HdrCbData *hdrcbdata = &per->hdrcbdata;
  struct OutStruct *outs = &per->outs;
  struct OutStruct *heads = &per->heads;
  struct OutStruct *etag_save = &per->etag_save;
  const char *str = ptr;
  const size_t cb = size * nmemb;
  const char *end = (char *)ptr + cb;
  const char *scheme = NULL;

  /*
   * Once that libcurl has called back tool_header_cb() the returned value
   * is checked against the amount that was intended to be written, if
   * it does not match then it fails with CURLE_WRITE_ERROR. So at this
   * point returning a value different from sz*nmemb indicates failure.
   */
  size_t failure = (size && nmemb) ? 0 : 1;

  if(!per->config)
    return failure;

#ifdef DEBUGBUILD
  if(size * nmemb > (size_t)CURL_MAX_HTTP_HEADER) {
    warnf(per->config->global, "Header data exceeds single call write "
          "limit!\n");
    return failure;
  }
#endif

  /*
   * Write header data when curl option --dump-header (-D) is given.
   */

  if(per->config->headerfile && heads->stream) {
    size_t rc = fwrite(ptr, size, nmemb, heads->stream);
    if(rc != cb)
      return rc;
    /* flush the stream to send off what we got earlier */
    (void)fflush(heads->stream);
  }

  /*
   * Write etag to file when --etag-save option is given.
   */
  if(per->config->etag_save_file && etag_save->stream) {
    /* match only header that start with etag (case insensitive) */
    if(curl_strnequal(str, "etag:", 5)) {
      const char *etag_h = &str[5];
      const char *eot = end - 1;
      if(*eot == '\n') {
        while(ISBLANK(*etag_h) && (etag_h < eot))
          etag_h++;
        while(ISSPACE(*eot))
          eot--;

        if(eot >= etag_h) {
          size_t etag_length = eot - etag_h + 1;
          fwrite(etag_h, size, etag_length, etag_save->stream);
          /* terminate with newline */
          fputc('\n', etag_save->stream);
          (void)fflush(etag_save->stream);
        }
      }
    }
  }

  /*
   * This callback sets the filename where output shall be written when
   * curl options --remote-name (-O) and --remote-header-name (-J) have
   * been simultaneously given and additionally server returns an HTTP
   * Content-Disposition header specifying a filename property.
   */

  curl_easy_getinfo(per->curl, CURLINFO_SCHEME, &scheme);
  scheme = proto_token(scheme);
  if(hdrcbdata->honor_cd_filename &&
     (cb > 20) && checkprefix("Content-disposition:", str) &&
     (scheme == proto_http || scheme == proto_https)) {
    const char *p = str + 20;

    /* look for the 'filename=' parameter
       (encoded filenames (*=) are not supported) */
    for(;;) {
      char *filename;
      size_t len;

      while(*p && (p < end) && !ISALPHA(*p))
        p++;
      if(p > end - 9)
        break;

      if(memcmp(p, "filename=", 9)) {
        /* no match, find next parameter */
        while((p < end) && (*p != ';'))
          p++;
        continue;
      }
      p += 9;

      /* this expression below typecasts 'cb' only to avoid
         warning: signed and unsigned type in conditional expression
      */
      len = (ssize_t)cb - (p - str);
      filename = parse_filename(p, len);
      if(filename) {
        if(outs->stream) {
          /* indication of problem, get out! */
          free(filename);
          return failure;
        }

        outs->is_cd_filename = TRUE;
        outs->s_isreg = TRUE;
        outs->fopened = FALSE;
        outs->filename = filename;
        outs->alloc_filename = TRUE;
        hdrcbdata->honor_cd_filename = FALSE; /* done now! */
        if(!tool_create_output_file(outs, per->config))
          return failure;
      }
      break;
    }
    if(!outs->stream && !tool_create_output_file(outs, per->config))
      return failure;
  }
  if(hdrcbdata->config->writeout) {
    char *value = memchr(ptr, ':', cb);
    if(value) {
      if(per->was_last_header_empty)
        per->num_headers = 0;
      per->was_last_header_empty = FALSE;
      per->num_headers++;
    }
    else if(ptr[0] == '\r' || ptr[0] == '\n')
      per->was_last_header_empty = TRUE;
  }
  if(hdrcbdata->config->show_headers &&
    (scheme == proto_http || scheme == proto_https ||
     scheme == proto_rtsp || scheme == proto_file)) {
    /* bold headers only for selected protocols */
    char *value = NULL;

    if(!outs->stream && !tool_create_output_file(outs, per->config))
      return failure;

    if(hdrcbdata->global->isatty && hdrcbdata->global->styled_output)
      value = memchr(ptr, ':', cb);
    if(value) {
      size_t namelen = value - ptr;
      fprintf(outs->stream, BOLD "%.*s" BOLDOFF ":", namelen, ptr);
#ifndef LINK
      fwrite(&value[1], cb - namelen - 1, 1, outs->stream);
#else
      if(curl_strnequal("Location", ptr, namelen)) {
        write_linked_location(per->curl, &value[1], cb - namelen - 1,
            outs->stream);
      }
      else
        fwrite(&value[1], cb - namelen - 1, 1, outs->stream);
#endif
    }
    else
      /* not "handled", just show it */
      fwrite(ptr, cb, 1, outs->stream);
  }
  return cb;
}

/*
 * Copies a file name part and returns an ALLOCATED data buffer.
 */
static char *parse_filename(const char *ptr, size_t len)
{
  char *copy;
  char *p;
  char *q;
  char  stop = '\0';

  /* simple implementation of strndup() */
  copy = malloc(len + 1);
  if(!copy)
    return NULL;
  memcpy(copy, ptr, len);
  copy[len] = '\0';

  p = copy;
  if(*p == '\'' || *p == '"') {
    /* store the starting quote */
    stop = *p;
    p++;
  }
  else
    stop = ';';

  /* scan for the end letter and stop there */
  q = strchr(p, stop);
  if(q)
    *q = '\0';

  /* if the filename contains a path, only use filename portion */
  q = strrchr(p, '/');
  if(q) {
    p = q + 1;
    if(!*p) {
      Curl_safefree(copy);
      return NULL;
    }
  }

  /* If the filename contains a backslash, only use filename portion. The idea
     is that even systems that don't handle backslashes as path separators
     probably want the path removed for convenience. */
  q = strrchr(p, '\\');
  if(q) {
    p = q + 1;
    if(!*p) {
      Curl_safefree(copy);
      return NULL;
    }
  }

  /* make sure the file name doesn't end in \r or \n */
  q = strchr(p, '\r');
  if(q)
    *q = '\0';

  q = strchr(p, '\n');
  if(q)
    *q = '\0';

  if(copy != p)
    memmove(copy, p, strlen(p) + 1);

#if defined(MSDOS) || defined(WIN32)
  {
    char *sanitized;
    SANITIZEcode sc = sanitize_file_name(&sanitized, copy, 0);
    Curl_safefree(copy);
    if(sc)
      return NULL;
    copy = sanitized;
  }
#endif /* MSDOS || WIN32 */

  /* in case we built debug enabled, we allow an environment variable
   * named CURL_TESTDIR to prefix the given file name to put it into a
   * specific directory
   */
#ifdef DEBUGBUILD
  {
    char *tdir = curlx_getenv("CURL_TESTDIR");
    if(tdir) {
      char buffer[512]; /* suitably large */
      msnprintf(buffer, sizeof(buffer), "%s/%s", tdir, copy);
      Curl_safefree(copy);
      copy = strdup(buffer); /* clone the buffer, we don't use the libcurl
                                aprintf() or similar since we want to use the
                                same memory code as the "real" parse_filename
                                function */
      curl_free(tdir);
    }
  }
#endif

  return copy;
}

#ifdef LINK
/*
 * Treat the Location: header specially, by writing a special escape
 * sequence that adds a hyperlink to the displayed text. This makes
 * the absolute URL of the redirect clickable in supported terminals,
 * which couldn't happen otherwise for relative URLs. The Location:
 * header is supposed to always be absolute so this theoretically
 * shouldn't be needed but the real world returns plenty of relative
 * URLs here.
 */
static
void write_linked_location(CURL *curl, const char *location, size_t loclen,
                           FILE *stream) {
  /* This would so simple if CURLINFO_REDIRECT_URL were available here */
  CURLU *u = NULL;
  char *copyloc = NULL, *locurl = NULL, *scheme = NULL, *finalurl = NULL;
  const char *loc = location;
  size_t llen = loclen;

  /* Strip leading whitespace of the redirect URL */
  while(llen && *loc == ' ') {
    ++loc;
    --llen;
  }

  /* Strip the trailing end-of-line characters, normally "\r\n" */
  while(llen && (loc[llen-1] == '\n' || loc[llen-1] == '\r'))
    --llen;

  /* CURLU makes it easy to handle the relative URL case */
  u = curl_url();
  if(!u)
    goto locout;

  /* Create a NUL-terminated and whitespace-stripped copy of Location: */
  copyloc = malloc(llen + 1);
  if(!copyloc)
    goto locout;
  memcpy(copyloc, loc, llen);
  copyloc[llen] = 0;

  /* The original URL to use as a base for a relative redirect URL */
  if(curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &locurl))
    goto locout;
  if(curl_url_set(u, CURLUPART_URL, locurl, 0))
    goto locout;

  /* Redirected location. This can be either absolute or relative. */
  if(curl_url_set(u, CURLUPART_URL, copyloc, 0))
    goto locout;

  if(curl_url_get(u, CURLUPART_URL, &finalurl, CURLU_NO_DEFAULT_PORT))
    goto locout;

  if(curl_url_get(u, CURLUPART_SCHEME, &scheme, 0))
    goto locout;

  if(!strcmp("http", scheme) ||
     !strcmp("https", scheme) ||
     !strcmp("ftp", scheme) ||
     !strcmp("ftps", scheme)) {
    fprintf(stream, LINK "%s" LINKST "%.*s" LINKOFF,
            finalurl, loclen, location);
    goto locdone;
  }

  /* Not a "safe" URL: don't linkify it */

locout:
  /* Write the normal output in case of error or unsafe */
  fwrite(location, loclen, 1, stream);

locdone:
  if(u) {
    curl_free(finalurl);
    curl_free(scheme);
    curl_url_cleanup(u);
    free(copyloc);
  }
}
#endif
