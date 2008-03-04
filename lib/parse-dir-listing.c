/*****************************************************************************/
/*  parse-dir-listing.c - contains functions for parsing the different types */
/*     of directory listings.                                                */
/*  Copyright (C) 1998-2008 Brian Masney <masneyb@gftp.org>                  */
/*                                                                           */
/*  This program is free software; you can redistribute it and/or modify     */
/*  it under the terms of the GNU General Public License as published by     */
/*  the Free Software Foundation; either version 2 of the License, or        */
/*  (at your option) any later version.                                      */
/*                                                                           */
/*  This program is distributed in the hope that it will be useful,          */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of           */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            */
/*  GNU General Public License for more details.                             */
/*                                                                           */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program; if not, write to the Free Software              */
/*  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111 USA      */
/*****************************************************************************/

#include "gftp.h"
static const char cvsid[] = "$Id: protocols.c 952 2008-01-24 23:31:26Z masneyb $";

static char *
copy_token (/*@out@*/ char **dest, char *source)
{
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


static char *
goto_next_token (char *pos)
{
  while (*pos != ' ' && *pos != '\t' && *pos != '\0')
    pos++;

  while (*pos == ' ' || *pos == '\t')
    pos++;

  return (pos);
}


static time_t
parse_vms_time (char *str, char **endpos)
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


time_t
parse_time (char *str, char **endpos)
{
  struct tm curtime, *loctime;
  time_t t, ret;
  char *tmppos;
  size_t slen;
  int i, num;

  slen = strlen (str);
  memset (&curtime, 0, sizeof (curtime));
  curtime.tm_isdst = -1;

  if (slen > 4 && isdigit ((int) str[0]) && str[2] == '-' && 
      isdigit ((int) str[3]))
    {
      /* This is how DOS will return the date/time */
      /* 07-06-99  12:57PM */

      tmppos = strptime (str, "%m-%d-%y %I:%M%p", &curtime);
    }
  else if (slen > 4 && isdigit ((int) str[0]) && str[2] == '-' && 
           isalpha (str[3]))
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


static mode_t
gftp_parse_vms_attribs (char **src, mode_t mask)
{
  char *endpos;
  mode_t ret;

  if (*src == NULL)
    return (0);

  if ((endpos = strchr (*src, ',')) != NULL)
    *endpos = '\0';

  ret = 0;
  if (strchr (*src, 'R') != NULL)
    ret |= S_IRUSR | S_IRGRP | S_IROTH;
  if (strchr (*src, 'W') != NULL)
    ret |= S_IWUSR | S_IWGRP | S_IWOTH;
  if (strchr (*src, 'E') != NULL)
    ret |= S_IXUSR | S_IXGRP | S_IXOTH;

  *src = endpos + 1;

  return (ret & mask);
}


static int
gftp_parse_ls_vms (gftp_request * request, int fd, char *str, gftp_file * fle)
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

  if ((curpos = strchr (str, ';')) == NULL)
    return (GFTP_EFATAL);

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
      if (request->get_next_dirlist_line == NULL)
        return (GFTP_EFATAL);

      len = request->get_next_dirlist_line (request, fd, tempstr,
                                            sizeof (tempstr));
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

  fle->st_mode = gftp_parse_vms_attribs (&curpos, S_IRWXU);
  fle->st_mode |= gftp_parse_vms_attribs (&curpos, S_IRWXG);
  fle->st_mode |= gftp_parse_vms_attribs (&curpos, S_IRWXO);

  fle->user = g_strdup ("");
  fle->group = g_strdup ("");

  return (0);
}


static int
gftp_parse_ls_mvs (char *str, gftp_file * fle)
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

  fle->user = g_strdup (_("unknown"));
  fle->group = g_strdup (_("unknown"));
  fle->file = g_strdup (curpos);

  return (0);
}
  

static int
gftp_parse_ls_eplf (char *str, gftp_file * fle)
{
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

  if ((startpos = strchr (str, 9)) == NULL)
    return (GFTP_EFATAL);

  if (isdir)
    fle->st_mode = S_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
  else
    fle->st_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

  fle->file = g_strdup (startpos + 1);
  fle->user = g_strdup (_("unknown"));
  fle->group = g_strdup (_("unknown"));
  return (0);
}


static int
gftp_parse_ls_unix (gftp_request * request, char *str, size_t slen,
                    gftp_file * fle)
{
  char *endpos, *startpos, *pos, *attribs;
  int cols;

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
      fle->group = g_strdup (_("unknown"));
      if (cols == 8)
        {
          if ((startpos = copy_token (&fle->user, startpos)) == NULL)
            return (GFTP_EFATAL);
        }
      else
        fle->user = g_strdup (_("unknown"));
      startpos = goto_next_token (startpos);
    }

  if (request->server_type == GFTP_DIRTYPE_CRAY)
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

  /* Uncomment this if you want to strip the spaces off of the end of the file.
     I don't want to do this by default since there are valid filenames with
     spaces at the end of them. Some broken FTP servers like the Paradyne IPC
     DSLAMS append a bunch of spaces at the end of the file.
  for (endpos = fle->file + strlen (fle->file) - 1; 
       *endpos == ' '; 
       *endpos-- = '\0');
  */

  return (0);
}


static int
gftp_parse_ls_nt (char *str, gftp_file * fle)
{
  char *startpos;

  startpos = str;
  fle->datetime = parse_time (startpos, &startpos);

  fle->user = g_strdup (_("unknown"));
  fle->group = g_strdup (_("unknown"));

  startpos = goto_next_token (startpos);

  if (startpos[0] == '<')
    fle->st_mode = S_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
  else
    {
      fle->st_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
      fle->size = gftp_parse_file_size (startpos);
    }

  startpos = goto_next_token (startpos);
  fle->file = g_strdup (startpos);
  return (0);
}


static int
gftp_parse_ls_novell (char *str, gftp_file * fle)
{
  char *startpos;

  if (str[12] != ' ')
    return (GFTP_EFATAL);

  str[12] = '\0';
  fle->st_mode = gftp_convert_attributes_to_mode_t (str);
  startpos = str + 13;

  while ((*startpos == ' ' || *startpos == '\t') && *startpos != '\0')
    startpos++;

  if ((startpos = copy_token (&fle->user, startpos)) == NULL)
    return (GFTP_EFATAL);

  fle->group = g_strdup (_("unknown"));

  while (*startpos != '\0' && !isdigit (*startpos))
    startpos++;

  fle->size = gftp_parse_file_size (startpos);

  startpos = goto_next_token (startpos);
  fle->datetime = parse_time (startpos, &startpos);

  startpos = goto_next_token (startpos);
  fle->file = g_strdup (startpos);
  return (0);
}


int
gftp_parse_ls (gftp_request * request, const char *lsoutput, gftp_file * fle,
               int fd)
{
  char *str, *endpos, tmpchar;
  int result, is_vms;
  size_t len;

  g_return_val_if_fail (lsoutput != NULL, GFTP_EFATAL);
  g_return_val_if_fail (fle != NULL, GFTP_EFATAL);

  str = g_strdup (lsoutput);
  memset (fle, 0, sizeof (*fle));

  len = strlen (str);
  if (len > 0 && str[len - 1] == '\n')
    str[--len] = '\0';
  if (len > 0 && str[len - 1] == '\r')
    str[--len] = '\0';

  switch (request->server_type)
    {
      case GFTP_DIRTYPE_CRAY:
      case GFTP_DIRTYPE_UNIX:
        result = gftp_parse_ls_unix (request, str, len, fle);
        break;
      case GFTP_DIRTYPE_EPLF:
        result = gftp_parse_ls_eplf (str, fle);
        break;
      case GFTP_DIRTYPE_NOVELL:
        result = gftp_parse_ls_novell (str, fle);
        break;
      case GFTP_DIRTYPE_DOS:
        result = gftp_parse_ls_nt (str, fle);
        break;
      case GFTP_DIRTYPE_VMS:
        result = gftp_parse_ls_vms (request, fd, str, fle);
        break;
      case GFTP_DIRTYPE_MVS:
        result = gftp_parse_ls_mvs (str, fle);
        break;
      default: /* autodetect */
        if (*lsoutput == '+')
          result = gftp_parse_ls_eplf (str, fle);
        else if (isdigit ((int) str[0]) && str[2] == '-')
          result = gftp_parse_ls_nt (str, fle);
        else if (str[1] == ' ' && str[2] == '[')
          result = gftp_parse_ls_novell (str, fle);
        else
          {
            if ((endpos = strchr (str, ' ')) != NULL)
              {
                /* If the first token in the string has a ; in it, then */
                /* we'll assume that this is a VMS directory listing    */
                tmpchar = *endpos;
                *endpos = '\0';
                is_vms = strchr (str, ';') != NULL;
                *endpos = tmpchar;
              }
            else
              is_vms = 0;

            if (is_vms)
              result = gftp_parse_ls_vms (request, fd, str, fle);
            else
              result = gftp_parse_ls_unix (request, str, len, fle);
          }
        break;
    }
  g_free (str);

  return (result);
}

