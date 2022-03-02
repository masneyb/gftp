/***********************************************************************************/
/*  ftp-dir-listing.c - contains functions for parsing the different types         */
/*     of directory listings.                                                      */
/*  Copyright (C) 1998-2008 Brian Masney <masneyb@gftp.org>                        */
/*  Copyright (C) 2021 wdlkmpx <wdlkmpx@gmail.com>                                 */
/*                                                                                 */
/*  Permission is hereby granted, free of charge, to any person obtaining a copy   */
/*  of this software and associated documentation files (the "Software"), to deal  */
/*  in the Software without restriction, including without limitation the rights   */
/*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell      */
/*  copies of the Software, and to permit persons to whom the Software is          */
/*  furnished to do so, subject to the following conditions:                       */
/*                                                                                 */
/*  The above copyright notice and this permission notice shall be included in all */
/*  copies or substantial portions of the Software.                                */
/*                                                                                 */
/*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR     */
/*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,       */
/*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE    */
/*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER         */
/*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,  */
/*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE  */
/*  SOFTWARE.                                                                      */
/***********************************************************************************/

#include "gftp.h"
#include "protocol_ftp.h"

#ifdef GFTP_DEBUG
static void detect_msg (int dirtype, const char * msg);
#endif

/* Server types (used by FTP protocol from SYST command) */
enum
{
   FTP_DIRTYPE_MLSD   = 0,
   FTP_DIRTYPE_UNIX   = 1,
   FTP_DIRTYPE_DOS    = 2,
   FTP_DIRTYPE_EPLF   = 3,
   FTP_DIRTYPE_CRAY   = 4,
   FTP_DIRTYPE_NOVELL = 5,
   FTP_DIRTYPE_VMS    = 6,
   FTP_DIRTYPE_MVS    = 7,
};

void parse_syst_response (char * response_str, ftp_protocol_data * ftpdat)
{
   char *startpos, *endpos;
   int dt = 0;
   //startpos endpos
   //    |    |
   // 215 UNIX Type: L8
   // 215 Windows_NT
   startpos = strchr (response_str, ' ');
   if (!startpos) {
      return;
   }
   startpos++;
   endpos = strchr (startpos, ' ');
   if (endpos) {
      *endpos = '\0';
   }

   // dirtype hint, this will be used if gftp is unable to properly detect the dirtype
   if      (strcmp (startpos, "UNIX") == 0)    dt = FTP_DIRTYPE_UNIX;
   else if (strcmp (startpos, "VMS") == 0)     dt = FTP_DIRTYPE_VMS;
   else if (strcmp (startpos, "MVS") == 0)     dt = FTP_DIRTYPE_MVS;
   else if (strcmp (startpos, "OS/MVS") == 0)  dt = FTP_DIRTYPE_MVS;
   else if (strcmp (startpos, "NETWARE") == 0) dt = FTP_DIRTYPE_NOVELL;
   else if (strcmp (startpos, "CRAY") == 0)    dt = FTP_DIRTYPE_CRAY;
   else if (strcmp (startpos, "Windows_NT") == 0) dt = FTP_DIRTYPE_DOS;

   ftpdat->list_dirtype_hint = dt;
#ifdef GFTP_DEBUG
   detect_msg (dt, "% Dirtype HINT: ");
#endif
}


static int detect_dirtype (char *string)
{
    if (!string || !*string) {
        return -1;
    }
    char * p;
    size_t slen = strlen (string);

    if (slen > 12)
    {
        p = strchr (string, ';');
        if (string[0] != ' ' && p && strstr(p+1,"; ")) {
            if (strstr(string,"ype=") && strstr(string,"odify=")) {
                //type= Type= modify= Modify=
                return FTP_DIRTYPE_MLSD;
            }
        }

        if (*string == '+') {
            return FTP_DIRTYPE_EPLF;
        }

        if ( (isdigit((int)string[0]) && isdigit((int)string[1]) && string[2] == '-')
                || (strstr(string, "     <DIR>     ")) ) {
            return FTP_DIRTYPE_DOS;
        }

        if (string[1] == ' ' && string[2] == '[' && string[12] == ' ') {
            return FTP_DIRTYPE_NOVELL;
        }

        if ((string[0] == '-' || string[0] == 'd' || string[0] == 'l') && (string[10] == ' ')) {
            /* 0123456789 */
            // drwxr-xr-x  8 user user   4096 Feb 16 12:25 .git
            // -rw-r--r--  1 user user    648 Nov  8 07:29 .gitignore
            string[10] = '\0';
            p = &string[1];
            if (strchr(p,'r') || strchr(p,'w') || strchr(p,'x') || strchr(p,'-')) {
                string[10] = ' ';
                return FTP_DIRTYPE_UNIX;
            }
            string[10] = ' ';
        }
    }

    p = strchr (string, ' ');
    if (p)
    {
        //If the first token in the string has a ; in it, then
        //we'll assume that this is a VMS directory listing
        *p = '\0';
        //If the first token in the string has a ; in it, then
        //we'll assume that this is a VMS directory listing
        if (strchr (string, ';')) {
            *p = ' ';
            return FTP_DIRTYPE_VMS;
        }
        *p = ' ';
    }

    return -2; /* could not detect dirtype */
}


#ifdef GFTP_DEBUG
static void detect_msg (int dirtype, const char * msg)
{
    if (msg) printf ("%s", msg);
    switch (dirtype)
    {
        case FTP_DIRTYPE_MLSD: printf ("MLSD\n"); break;
        case FTP_DIRTYPE_CRAY: printf ("CRAY\n"); break;
        case FTP_DIRTYPE_UNIX: printf ("UNIX\n"); break;
        case FTP_DIRTYPE_EPLF: printf ("EPLF\n"); break;
        case FTP_DIRTYPE_NOVELL: printf ("NOVEL\n"); break;
        case FTP_DIRTYPE_DOS: printf ("DOS\n"); break;
        case FTP_DIRTYPE_VMS: printf ("VMS\n"); break;
        case FTP_DIRTYPE_MVS: printf ("MVS\n"); break;
        default:  printf ("[none]\n"); break;
    }
}
#endif

// =========================================================================
// helper functions for gftp_parse_*

static char * copy_token (char **dest, char *source)
{                         /*@out@*/
  char *endpos, savepos;

  endpos = source;
  while (*endpos != ' ' && *endpos != '\t' && *endpos != '\0')
    endpos++;
  if (*endpos == '\0')
    {
      *dest = NULL;
      return (NULL);
    }

  savepos = *endpos;
  *endpos = '\0';
  *dest = g_malloc0 ((gulong) (endpos - source + 1));
  strcpy (*dest, source);
  *endpos = savepos;

  /* Skip the blanks till we get to the next entry */
  source = endpos + 1;
  while ((*source == ' ' || *source == '\t') && *source != '\0')
    source++;
  return (source);
}


static char * goto_next_token (char *pos)
{
  while (*pos != ' ' && *pos != '\t' && *pos != '\0') {
    pos++;
  }
  while (*pos == ' ' || *pos == '\t') {
    pos++;
  }
  return (pos);
}


static time_t parse_vms_time (char *str, char **endpos)
{
  struct tm curtime;
  time_t ret;

  /* 8-JUN-2004 13:04:14 */
  memset (&curtime, 0, sizeof (curtime));

  *endpos = strptime (str, "%d-%b-%Y %H:%M:%S", &curtime);
  if (*endpos == NULL)
    *endpos = strptime (str, "%d-%b-%Y %H:%M", &curtime);
  
  if (*endpos != NULL)
    {
      ret = mktime (&curtime);
      for (; **endpos == ' ' || **endpos == '\t'; (*endpos)++);
    }
  else
    {
      ret = 0;
      *endpos = goto_next_token (str);
      if (*endpos != NULL)
        *endpos = goto_next_token (*endpos);
    }

  return (ret);
}


time_t parse_time (char *str, char **endpos)
{
  struct tm curtime, *loctime;
  time_t t, ret;
  char *tmppos;
  size_t slen;
  int i, num;

  slen = strlen (str);
  memset (&curtime, 0, sizeof (curtime));
  curtime.tm_isdst = -1;

  if (slen > 4 && isdigit ((int) str[0]) && str[2] == '-' && isdigit ((int) str[3]))
    {
      /* This is how DOS will return the date/time */
      /* 07-06-99  12:57PM */
      tmppos = strptime (str, "%m-%d-%y %I:%M%p", &curtime);
    }
  else if (slen > 4 && isdigit ((int) str[0]) && str[2] == '-' && isalpha (str[3]))
    {
      /* 10-Jan-2003 09:14 */
      tmppos = strptime (str, "%d-%h-%Y %H:%M", &curtime);
    }
  else if (slen > 4 && isdigit ((int) str[0]) && str[4] == '/')
    {
      /* 2003/12/25 */
      tmppos = strptime (str, "%Y/%m/%d", &curtime);
    }
  else
    {
      /* This is how most UNIX, Novell, and MacOS ftp servers send their time */
      /* Jul 06 12:57 or Jul  6  1999 */
      if (strchr (str, ':') != NULL)
        {
          tmppos = strptime (str, "%h %d %H:%M", &curtime);
          t = time (NULL);
          loctime = localtime (&t);

          if (curtime.tm_mon > loctime->tm_mon)
            curtime.tm_year = loctime->tm_year - 1;
          else
            curtime.tm_year = loctime->tm_year;
        }
      else
        tmppos = strptime (str, "%h %d %Y", &curtime);
    }

  if (tmppos != NULL)
    ret = mktime (&curtime);
  else
    ret = 0;

  if (endpos != NULL)
    {
      if (tmppos == NULL)
        {
          /* We cannot parse this date format. So, just skip this date field
             and continue to the next token. This is mainly for the HTTP 
             support */

          *endpos = str;
          for (num = 0; num < 2 && **endpos != '\0'; num++)
            {
              for (i=0; 
                   (*endpos)[i] != ' ' && (*endpos)[i] != '\t' && 
                    (*endpos)[i] != '\0'; 
                   i++);
              *endpos += i;

              for (i=0; (*endpos)[i] == ' ' || (*endpos)[i] == '\t'; i++);
              *endpos += i;
            }
        }
      else
        *endpos = tmppos;
    }

  return (ret);
}


// =========================================================================


static mode_t ftp_parse_vms_attribs (char **src, mode_t mask)
{
  char *endpos;
  mode_t ret;

  if (*src == NULL)
    return (0);

  if ((endpos = strchr (*src, ',')) != NULL) {
    *endpos = '\0';
  }
  ret = 0;
  if (strchr (*src, 'R')) ret |= S_IRUSR | S_IRGRP | S_IROTH;
  if (strchr (*src, 'W')) ret |= S_IWUSR | S_IWGRP | S_IWOTH;
  if (strchr (*src, 'E')) ret |= S_IXUSR | S_IXGRP | S_IXOTH;
  *src = endpos + 1;

  return (ret & mask);
}


static int ftp_parse_ls_vms (gftp_request * request, int fd, char *str, gftp_file * fle)
{
  char *curpos, *endpos, tempstr[1024];
  int multiline;
  ssize_t len;

  /* .PINE-DEBUG1;1              9  21-AUG-2002 20:06 [MYERSRG] (RWED,RWED,,) */
  /* WWW.DIR;1                   1  23-NOV-1999 05:47 [MYERSRG] (RWE,RWE,RE,E) */

  /* Multiline VMS 
  $MAIN.TPU$JOURNAL;1
	1/18 8-JUN-2004 13:04:14  [NUCLEAR,FISSION]      (RWED,RWED,RE,)
  TCPIP$FTP_SERVER.LOG;29
	0/18 8-JUN-2004 14:42:04  [NUCLEAR,FISSION]      (RWED,RWED,RE,)
  TCPIP$FTP_SERVER.LOG;28
	5/18 8-JUN-2004 13:05:11  [NUCLEAR,FISSION]      (RWED,RWED,RE,)
  TCPIP$FTP_SERVER.LOG;27
	5/18 8-JUN-2004 13:03:51  [NUCLEAR,FISSION]      (RWED,RWED,RE,) */

  if ((curpos = strchr (str, ';')) == NULL) {
    return (GFTP_EFATAL);
  }
  multiline = strchr (str, ' ') == NULL;

  *curpos = '\0';
  if (strlen (str) > 4 && strcmp (curpos - 4, ".DIR") == 0)
    {
      fle->st_mode |= S_IFDIR;
      *(curpos - 4) = '\0';
    }

  fle->file = g_strdup (str);

  if (multiline)
    {
      len = ftp_get_next_dirlist_line (request, fd, tempstr, sizeof (tempstr));
      if (len <= 0)
        return ((int) len);

      for (curpos = tempstr; *curpos == ' ' || *curpos == '\t'; curpos++);
    }
  else
    curpos = goto_next_token (curpos + 1);

  fle->size = gftp_parse_file_size (curpos) * 512; /* Is this correct? */

  curpos = goto_next_token (curpos);

  fle->datetime = parse_vms_time (curpos, &curpos);

  if (*curpos != '[')
    return (GFTP_EFATAL);

  if ((endpos = strchr (curpos, ']')) == NULL)
    return (GFTP_EFATAL);

  curpos = goto_next_token (endpos + 1);
  if ((curpos = strchr (curpos, ',')) == NULL)
    return (0);
  curpos++;

  fle->st_mode = ftp_parse_vms_attribs (&curpos, S_IRWXU);
  fle->st_mode |= ftp_parse_vms_attribs (&curpos, S_IRWXG);
  fle->st_mode |= ftp_parse_vms_attribs (&curpos, S_IRWXO);

  fle->user  = strdup ("-"); /* unknown */
  fle->group = strdup ("-"); /* unknown */

  return (0);
}


static int ftp_parse_ls_mvs (char *str, gftp_file * fle)
{
  char *curpos;

  /* Volume Unit    Referred Ext Used Recfm Lrecl BlkSz Dsorg Dsname */
  /* SVI52A 3390   2003/12/10  8  216  FB      80 27920  PS  CARDS.DELETES */
  /* SVI528 3390   2003/12/12  1    5  FB      80 24000  PO  CLIST */

  curpos = goto_next_token (str + 1);
  if (curpos == NULL)
    return (GFTP_EFATAL);

  curpos = goto_next_token (curpos + 1);
  if (curpos == NULL)
    return (GFTP_EFATAL);

  fle->datetime = parse_time (curpos, &curpos);

  curpos = goto_next_token (curpos);
  if (curpos == NULL)
    return (GFTP_EFATAL);

  curpos = goto_next_token (curpos + 1);
  if (curpos == NULL)
    return (GFTP_EFATAL);

  fle->size = gftp_parse_file_size (curpos) * 55996; 
  curpos = goto_next_token (curpos + 1);
  if (curpos == NULL)
    return (GFTP_EFATAL);

  curpos = goto_next_token (curpos + 1);
  if (curpos == NULL)
    return (GFTP_EFATAL);

  curpos = goto_next_token (curpos + 1);
  if (curpos == NULL)
    return (GFTP_EFATAL);

  curpos = goto_next_token (curpos + 1);
  if (curpos == NULL)
    return (GFTP_EFATAL);

  if (strncmp (curpos, "PS", 2) == 0)
    fle->st_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
  else if (strncmp (curpos, "PO", 2) == 0)
    fle->st_mode = S_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
  else
    return (GFTP_EFATAL);

  curpos = goto_next_token (curpos + 1);

  fle->user  = strdup ("-"); /* unknown */
  fle->group = strdup ("-"); /* unknown */
  fle->file = g_strdup (curpos);

  return (0);
}
  

static int ftp_parse_ls_eplf (char *str, gftp_file * fle)
{
  // +i8388621.48594,m825718503,r,s280, djb.html
  char *startpos;
  int isdir = 0;

  startpos = str;
  while (startpos)
    {
      startpos++;
      switch (*startpos)
        {
          case '/':
            isdir = 1;
            break;
          case 's':
            fle->size = gftp_parse_file_size (startpos + 1);
            break;
          case 'm':
            fle->datetime = strtol (startpos + 1, NULL, 10);
            break;
        }
      startpos = strchr (startpos, ',');
    }

  if ((startpos = strchr (str, 9)) == NULL) {
    return (GFTP_EFATAL);
  }
  if (isdir) {
    fle->st_mode = S_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
  } else {
    fle->st_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
  }
  fle->file = g_strdup (startpos + 1);
  fle->user  = strdup ("-"); /* unknown */
  fle->group = strdup ("-"); /* unknown */
  return (0);
}


static int ftp_parse_ls_unix (gftp_request * request, char *str, size_t slen,
                               gftp_file * fle)
{
  //DEBUG_TRACE("LSUNIX: %s\n", str);
  char *endpos, *startpos, *pos, *attribs;
  int cols;
  ftp_protocol_data * ftpdat = request->protocol_data;

  /* If there is no space between the attribs and links field, just make one */
  if (slen > 10)
    str[10] = ' ';

  /* Determine the number of columns */
  cols = 0;
  pos = str;
  while (*pos != '\0')
    {
      while (*pos != '\0' && *pos != ' ' && *pos != '\t')
        {
          if (*pos == ':')
            break;
          pos++;
        }

      cols++;

      if (*pos == ':')
        {
          cols++;
          break;
        }

      while (*pos == ' ' || *pos == '\t')
        pos++;
    }

  startpos = str;
  /* Copy file attributes */
  if ((startpos = copy_token (&attribs, startpos)) == NULL)
    return (GFTP_EFATAL);

  if (strlen (attribs) < 10)
    return (GFTP_EFATAL);

  fle->st_mode = gftp_convert_attributes_to_mode_t (attribs);
  g_free (attribs);

  if (cols >= 9)
    {
      /* Skip the number of links */
      startpos = goto_next_token (startpos);

      /* Copy the user that owns this file */
      if ((startpos = copy_token (&fle->user, startpos)) == NULL)
        return (GFTP_EFATAL);

      /* Copy the group that owns this file */
      if ((startpos = copy_token (&fle->group, startpos)) == NULL)
        return (GFTP_EFATAL);
    }
  else
    {
      fle->group = strdup ("-"); /* unknown */
      if (cols == 8)
        {
          if ((startpos = copy_token (&fle->user, startpos)) == NULL)
            return (GFTP_EFATAL);
        }
      else
        fle->user  = strdup ("-"); /* unknown */
      startpos = goto_next_token (startpos);
    }
 
  if (ftpdat->list_dirtype_hint == FTP_DIRTYPE_CRAY)
    {
      /* See if this is a Cray directory listing. It has the following format:
      drwx------     2 feiliu    g913     DK  common      4096 Sep 24  2001 wv */
      if (cols == 11 && strstr (str, "->") == NULL)
        {
          startpos = goto_next_token (startpos);
          startpos = goto_next_token (startpos);
        }
    }

  /* See if this is a block or character device. We will store the major number
     in the high word and the minor number in the low word.  */
  if (GFTP_IS_SPECIAL_DEVICE (fle->st_mode) &&
      (endpos = strchr (startpos, ',')) != NULL)
    {
      fle->size = (unsigned long) strtol (startpos, NULL, 10) << 16;

      startpos = endpos + 1;
      while (*startpos == ' ')
        startpos++;

      /* Get the minor number */
      if ((endpos = strchr (startpos, ' ')) == NULL)
        return (GFTP_EFATAL);
      fle->size |= strtol (startpos, NULL, 10) & 0xFF;
    }
  else
    {
      /* This is a regular file  */
      if ((endpos = strchr (startpos, ' ')) == NULL)
        return (GFTP_EFATAL);
      fle->size = gftp_parse_file_size (startpos);
    }

  /* Skip the blanks till we get to the next entry */
  startpos = endpos + 1;
  while (*startpos == ' ')
    startpos++;

  fle->datetime = parse_time (startpos, &startpos);

  /* Skip the blanks till we get to the next entry */
  startpos = goto_next_token (startpos);

  /* Parse the filename. If this file is a symbolic link, remove the -> part */
  if (S_ISLNK (fle->st_mode) && ((endpos = strstr (startpos, "->")) != NULL))
    *(endpos - 1) = '\0';

  fle->file = g_strdup (startpos);

  return (0);
}


static int ftp_parse_ls_nt (char *str, gftp_file * fle)
{
  /* 0123456789... */
  // 12-03-15  08:14PM       <DIR>          aspnet_client
  // 10-19-20  03:19PM       <DIR>          pub
  // 04-08-14  03:09PM                  403 readme.txt
  char *startpos;

  startpos = str;
  fle->datetime = parse_time (startpos, &startpos);

  fle->user  = strdup ("-"); /* unknown */
  fle->group = strdup ("-"); /* unknown */

  startpos = goto_next_token (startpos);

  if (startpos[0] == '<')
    fle->st_mode = S_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
  else
    {
      fle->st_mode = S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
      fle->size = gftp_parse_file_size (startpos);
    }

  startpos = goto_next_token (startpos);
  fle->file = g_strdup (startpos);
  return (0);
}


static int ftp_parse_ls_novell (char *str, gftp_file * fle)
{
  /* NETWARE */
  /* 0123456789... */
  // - [RWCE-FM-] bubble.ou=gum.ou=do..           41472 Mar 11  2003 my test map.doc
  // d [RWCE-FM-] bubble.ou=gum.ou=do..             512 Jan 15  2001 1997
  // d [RWCE-FM-] ego.ou=guest..                    512 Jan 31 15:05 OldStuff
  char *startpos;

  if (str[12] != ' ') {
    return (GFTP_EFATAL);
  }
  str[12] = '\0';
  fle->st_mode = gftp_convert_attributes_to_mode_t (str);
  startpos = str + 13;

  while ((*startpos == ' ' || *startpos == '\t') && *startpos != '\0') {
    startpos++;
  }
  if ((startpos = copy_token (&fle->user, startpos)) == NULL) {
    return (GFTP_EFATAL);
  }
  fle->group = strdup ("-"); /* unknown */

  while (*startpos != '\0' && !isdigit (*startpos)) {
    startpos++;
  }
  fle->size = gftp_parse_file_size (startpos);

  startpos = goto_next_token (startpos);
  fle->datetime = parse_time (startpos, &startpos);

  startpos = goto_next_token (startpos);
  fle->file = g_strdup (startpos);
  return (0);
}

// =========================================================================
//                             MLSD / MLST
// =========================================================================

static int ftp_parse_ls_mlsd (char *str, gftp_file * fle)
{
   // type=file;size=79514812;modify=20211025203440;UNIX.mode=0644;unique=10g2534; zz.deb
   // modify=20210930020540;perm=flcdmpe;type=dir;unique=36U22;UNIX.group=65534;UNIX.mode=0755;UNIX.owner=112; mrtg
   // modify=20180626065826;perm=adfr;size=400;type=file;unique=2DU104;UNIX.group=0;UNIX.mode=0644;UNIX.owner=0; HEADER.html
   //DEBUG_TRACE("MLSD: %s\n", str)
   int ntokens, i;
   char ** tokens;
   char * filename, * strtype, * strsize, * strmodify, * strperm;
   char * strunix_uid, * strunix_gid, * strunix_mode;
   char * strunix_owner, * strunix_group, * strunix_ownername, * strunix_groupname;
   char * p;
   struct tm stm;
   unsigned fmode = 0;

   filename = strtype = strsize = strmodify = strperm = NULL;
   strunix_uid = strunix_gid = strunix_mode = NULL;
   strunix_owner = strunix_group = strunix_ownername = strunix_groupname = NULL;

   // get filename
   p = strchr (str, ' ');
   if (p) {
      *p = 0;
      p++;
      //DEBUG_PUTS(p)
      if (*p) {
         /*NetBSD-ftpd sends the CWD in the first line with full path */
         // Type=cdir;Modify=20170930223132;Perm=el;Unique=AagAAAAAAADASAAAAAAAAA; /pub
         // Type=cdir;Modify=20161006172557;Perm=el;Unique=AagAAAAAAADESAAAAAAAAA; /pub/NetBSD-daily
         /*ftp_get_next_file() will discard it*/
         if (*p == '/') {
            fle->file = NULL;
            return 0;
         }
         filename = p;
      } else {
         return GFTP_EFATAL;
      }
   } else {
      return GFTP_EFATAL;
   }

   // determine number of tokens
   p = str;
   ntokens = 0;
   while (*p) {
      if (*p == ';') ntokens++;
      p++;
   }
   //DEBUG_TRACE("tokens: %d\n", ntokens)
   tokens = (char**) calloc (ntokens+1, sizeof(char**));

   // tokenize; assign tokens
   p = str;
   tokens[0] = str; // first token is already determined...
   i = 1;
   while (*p)
   {
      if (*p == ';') {
         *p = 0;
         p++;
         if (!*p) {
            break;
         }
         tokens[i] = p;
         i++;
         continue;
      }
      p++;
   }
   tokens[ntokens] = NULL;

   // assign str* variables
   for (i = 0; tokens[i]; i++)
   {
      //DEBUG_PUTS(tokens[i])
      if (strncasecmp (tokens[i], "type=", 5) == 0) {
         strtype = tokens[i] + 5;
         continue;
      }
      if (strncasecmp (tokens[i], "size=", 5) == 0) {
         strsize = tokens[i] + 5;
         continue;
      }
      if (strncasecmp (tokens[i], "perm=", 5) == 0) {
         strperm = tokens[i] + 5;
         continue;
      }
      if (strncasecmp (tokens[i], "modify=", 7) == 0) {
         strmodify = tokens[i] + 7;
         continue;
      }
      if (strncasecmp (tokens[i], "unix.mode=", 10) == 0) {
         strunix_mode = tokens[i] + 10;
         continue;
      }
      if (strncasecmp (tokens[i], "unix.uid=", 9) == 0) {
         strunix_uid = tokens[i] + 9;
         if (!*strunix_uid) strunix_uid = NULL;
         continue;
      }
      if (strncasecmp (tokens[i], "unix.gid=", 9) == 0) {
         strunix_gid = tokens[i] + 9;
         if (!*strunix_gid) strunix_gid = NULL;
         continue;
      }
      if (strncasecmp (tokens[i], "unix.owner=", 11) == 0) {
         strunix_owner = tokens[i] + 11;
         if (!*strunix_owner) strunix_owner = NULL;
         continue;
      }
      if (strncasecmp (tokens[i], "unix.group=", 11) == 0) {
         strunix_group = tokens[i] + 11;
         if (!*strunix_group) strunix_group = NULL;
         continue;
      }
      if (strncasecmp (tokens[i], "unix.ownername=", 15) == 0) {
         strunix_ownername = tokens[i] + 15;
         if (!*strunix_ownername) strunix_ownername = NULL;
         continue;
      }
      if (strncasecmp (tokens[i], "unix.groupname=", 15) == 0) {
         strunix_groupname = tokens[i] + 15;
         if (!*strunix_groupname) strunix_groupname = NULL;
         continue;
      }
   }
   free (tokens);

   if (!filename || !*filename || !strtype || !*strtype) {
      return GFTP_EFATAL;
   }

   // determine values for gftp_file
   fle->file = strdup (filename);

   if (strtype && *strtype) {
      // type=pdir
      if ((strcmp(strtype,"dir") == 0) || (strcmp(strtype,"pdir") == 0) || (strcmp(strtype,"cdir") == 0)) {
         fle->st_mode = S_IFDIR;
         fle->size = 4096; /* sizd */
      } else if (strstr (strtype, "link")) {
         fle->st_mode = S_IFLNK;
      } else {
         fle->st_mode = S_IFREG;
      }
   }

   if (strsize && *strsize) {
      // size=79514812
      fle->size = gftp_parse_file_size (strsize);
   }
   if (strmodify && *strmodify) {
      // modify=20211118041946
      //        Y---m-d-H-M-S-
      strptime (strmodify, "%Y%m%d%H%M%S", &stm);
      fle->datetime = mktime (&stm); // time_t
   }

   if (strunix_mode && *strunix_mode) {
      // UNIX.mode=0644
      sscanf (strunix_mode, "%o", &fmode);
      fle->st_mode |= fmode;
   } else if (strperm) {
      // perm=... doesn't map to UNIX permissions
      // so instead of wasting cpu time processing nonsensical permissions
      // just assign some default values...
      fle->st_mode |= 0755;
   } else {
      // set some good default permissions..
      fle->st_mode |= 0755;
   }

   if (strunix_ownername)  fle->user = strdup (strunix_ownername);
   else if (strunix_owner) fle->user = strdup (strunix_owner);
   else if (strunix_uid)   fle->user = strdup (strunix_uid);
   else                    fle->user = strdup ("-"); /* unknown */

   if (strunix_groupname)  fle->group = strdup (strunix_groupname);
   else if (strunix_group) fle->group = strdup (strunix_group);
   else if (strunix_gid)   fle->group = strdup (strunix_gid);
   else                    fle->group = strdup ("-"); /* unknown */

   return 0;
}


// =========================================================================
//            main function:   ftp_parse_ls()
// =========================================================================

int ftp_parse_ls (gftp_request * request, const char *lsoutput, gftp_file * fle,
               int fd)
{
  DEBUG_TRACE("LS(%d): %s\n", strlen(lsoutput), lsoutput)
  char *str;
  int result;
  size_t len;
  ftp_protocol_data * ftpdat;
  int dirtype;

  g_return_val_if_fail (lsoutput != NULL, GFTP_EFATAL);
  g_return_val_if_fail (fle != NULL, GFTP_EFATAL);

  ftpdat = request->protocol_data;
  str = g_strdup (lsoutput);
  memset (fle, 0, sizeof (*fle));

  len = strlen (str);
  if (len > 0 && str[len - 1] == '\n')
    str[--len] = '\0';
  if (len > 0 && str[len - 1] == '\r')
    str[--len] = '\0';

  // ----
  if (ftpdat->list_dirtype > -3 && ftpdat->list_dirtype < 0)
  {
      /* -1 is set before requesting file list */
      // try up 2 times to detect the dirtype: -2, -3
      // the 1st line may be the header - invalid
      dirtype = detect_dirtype (str);
#ifdef GFTP_DEBUG
      detect_msg (dirtype, "%% Dirtype DETECTED: ");
#endif
      if (dirtype >= 0) {
         ftpdat->list_dirtype = dirtype;
      } else {
         ftpdat->list_dirtype--;
      }
  }

  dirtype = ftpdat->list_dirtype;

  if (dirtype < 0) {
     dirtype = ftpdat->list_dirtype_hint;
  }
  // ----

  switch (dirtype)
  {
      case FTP_DIRTYPE_MLSD:
        result = ftp_parse_ls_mlsd (str, fle);
        break;
      case FTP_DIRTYPE_CRAY:
      case FTP_DIRTYPE_UNIX:
        result = ftp_parse_ls_unix (request, str, len, fle);
        break;
      case FTP_DIRTYPE_EPLF:
        result = ftp_parse_ls_eplf (str, fle);
        break;
      case FTP_DIRTYPE_NOVELL:
        result = ftp_parse_ls_novell (str, fle);
        break;
      case FTP_DIRTYPE_DOS:
        result = ftp_parse_ls_nt (str, fle);
        break;
      case FTP_DIRTYPE_VMS:
        result = ftp_parse_ls_vms (request, fd, str, fle);
        break;
      case FTP_DIRTYPE_MVS:
        result = ftp_parse_ls_mvs (str, fle);
        break;
      default:
        result = -1;
        break;
  }
  g_free (str);

  return (result);
}

