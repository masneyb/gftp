/*****************************************************************************/
/*  config_file.c - config file routines                                     */
/*  Copyright (C) 1998-2002 Brian Masney <masneyb@gftp.org>                  */
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
static const char cvsid[] = "$Id$";

static void write_comment 			( FILE * fd, 
						  const char *comment );
static int parse_args 				( char *str, 
						  int numargs, 
						  int lineno, 
						  char **first, 
						  ... );

gftp_config_vars config_file_vars[] = 
{
  {"", N_("General"), (void *) 0x1, CONFIG_NOTEBOOK, "", NULL, GFTP_PORT_GTK},
  {"", "", (void *) 0x1, CONFIG_TABLE, "", NULL, GFTP_PORT_GTK},
  {"email", N_("Email address:"), &emailaddr, CONFIG_CHARTEXT, 
	N_("Enter your email address here"), NULL, GFTP_PORT_ALL},
  {"view_program", N_("View program:"), &view_program, CONFIG_CHARTEXT,
	N_("The default program used to view files. If this is blank, the internal file viewer will be used"), NULL, GFTP_PORT_ALL},
  {"edit_program", N_("Edit program:"), &edit_program, CONFIG_CHARTEXT,
	N_("The default program used to edit files."), NULL, GFTP_PORT_GTK},
  {"startup_directory", N_("Startup Directory:"), &startup_directory, CONFIG_CHARTEXT,
        N_("The default directory gFTP will go to on startup"), NULL, GFTP_PORT_ALL},
  {"max_log_window_size", N_("Max Log Window Size:"), &max_log_window_size, CONFIG_UINTTEXT,
        N_("The maximum size of the log window in bytes for the GTK+ port"), NULL, GFTP_PORT_GTK},
  {"", "", (void *) 0x1, CONFIG_TABLE, "", NULL, GFTP_PORT_GTK},
  {"append_transfers", N_("Append file transfers"), &append_file_transfers,
        CONFIG_CHECKBOX,
        N_("Append new file transfers onto existing ones"), NULL, GFTP_PORT_GTK},
  {"one_transfer", N_("Do one transfer at a time"),  &do_one_transfer_at_a_time, CONFIG_CHECKBOX, 
	N_("Do only one transfer at a time?"), NULL, GFTP_PORT_GTK}, 
  {"overwrite_default", N_("Overwrite by Default"), &overwrite_by_default, CONFIG_CHECKBOX,
        N_("Overwrite files by default or set to resume file transfers"), NULL, GFTP_PORT_GTK},
  {"refresh_files", N_("Refresh after each file transfer"), &refresh_files, CONFIG_CHECKBOX, 
	N_("Refresh the listbox after each file is transfered"), NULL, GFTP_PORT_GTK},
  {"sort_dirs_first", N_("Sort directories first"), &sort_dirs_first, CONFIG_CHECKBOX, 
	N_("Put the directories first then the files"), NULL, GFTP_PORT_ALL},
  {"start_transfers", N_("Start file transfers"), &start_file_transfers, CONFIG_CHECKBOX, 
	N_("Automatically start the file transfers when they get queued?"), NULL, GFTP_PORT_GTK},
  {"show_hidden_files", N_("Show hidden files"), &show_hidden_files, CONFIG_CHECKBOX, 
	N_("Show hidden files in the listboxes"), NULL, GFTP_PORT_ALL},
  
  {"", N_("Network"), (void *) 0x1, CONFIG_NOTEBOOK, "", NULL, GFTP_PORT_GTK},
  {"", "", (void *) 0x1, CONFIG_TABLE, "", NULL, GFTP_PORT_GTK},
  {"network_timeout", N_("Network timeout:"), &network_timeout, CONFIG_INTTEXT, 
	N_("The timeout waiting for network input/output. This is NOT an idle timeout."), NULL, GFTP_PORT_ALL},
  {"retries", N_("Connect retries:"), &retries, CONFIG_INTTEXT, 
	N_("The number of auto-retries to do. Set this to 0 to retry indefinately"), NULL, GFTP_PORT_ALL},
  {"sleep_time", N_("Retry sleep time:"), &sleep_time, CONFIG_INTTEXT, 
	N_("The number of seconds to wait between retries"), NULL, GFTP_PORT_ALL},
  {"maxkbs", N_("Max KB/S:"), &maxkbs, CONFIG_FLOATTEXT, 
	N_("The maximum KB/s a file transfer can get. (Set to 0 to disable)"), NULL, GFTP_PORT_ALL},
  {"", N_("Default Protocol"), (void *) 0x1, CONFIG_COMBO, "DP", NULL, GFTP_PORT_GTK},
  {"default_protocol", N_("Default Protocol"), &default_protocol, 
        CONFIG_CHARTEXT, N_("This specifies the default protocol to use"), NULL, 0},

  {"", N_("FTP"), (void *) 0x1, CONFIG_NOTEBOOK, "", NULL, GFTP_PORT_GTK},
  {"", "", (void *) 0x1, CONFIG_TABLE, "", NULL, GFTP_PORT_GTK},
  {"firewall_host", N_("Proxy hostname:"), &firewall_host, CONFIG_CHARTEXT,
	N_("Firewall hostname"), NULL, GFTP_PORT_ALL},
  {"firewall_port", N_("Proxy port:"), &firewall_port, CONFIG_INTTEXT,
	N_("Port to connect to on the firewall"), NULL, GFTP_PORT_ALL},
  {"firewall_username", N_("Proxy username:"), &firewall_username,
	CONFIG_CHARTEXT, N_("Your firewall username"), NULL, GFTP_PORT_ALL},
  {"firewall_password", N_("Proxy password:"), &firewall_password, 
	CONFIG_CHARPASS, N_("Your firewall password"), NULL, GFTP_PORT_ALL},
  {"firewall_account", N_("Proxy account:"), &firewall_account, CONFIG_CHARTEXT, 
	N_("Your firewall account (optional)"), NULL, GFTP_PORT_ALL},
  {"", "", (void *) 0x1, CONFIG_TABLE, "", NULL, GFTP_PORT_GTK},

  {"passive_transfer", N_("Passive file transfers"), &passive_transfer, CONFIG_CHECKBOX, 
	N_("Send PASV command or PORT command for data transfers"), NULL, GFTP_PORT_ALL},
  {"resolve_symlinks", N_("Resolve Remote Symlinks (LIST -L)"), &resolve_symlinks, CONFIG_CHECKBOX, 
	N_("If you disable this feature, then gFTP will only send LIST to the remote server instead of LIST -L"), NULL, GFTP_PORT_ALL},

  {"", "", (void *) 0x1, CONFIG_TABLE, "", NULL, GFTP_PORT_GTK},
  {"", N_("Proxy server type"), (void *) 0x1, CONFIG_COMBO, "PS", NULL, GFTP_PORT_GTK},
  {"proxy_config", N_("Proxy config"), &proxy_config, CONFIG_TEXT,
	N_("This specifies how your proxy server expects us to log in"), NULL, GFTP_PORT_GTK},
  {"", N_("%pu = proxy user"), (void *) 0x1, CONFIG_LABEL, "", NULL, 0},
  {"", N_("%hu = host user"), (void *) 0x1, CONFIG_LABEL, "", NULL, 0},
  {"", N_("%pp = proxy pass"), (void *) 0x1, CONFIG_LABEL, "", NULL, 0},
  {"", N_("%hp = host pass"), (void *) 0x1, CONFIG_LABEL, "", NULL, 0},
  {"", N_("%ph = proxy host"), (void *) 0x1, CONFIG_LABEL, "", NULL, 0},
  {"", N_("%hh = host"), (void *) 0x1, CONFIG_LABEL, "", NULL, 0},
  {"", N_("%po = proxy port"), (void *) 0x1, CONFIG_LABEL, "", NULL, 0},
  {"", N_("%ho = host port"), (void *) 0x1, CONFIG_LABEL, "", NULL, 0},
  {"", N_("%pa = proxy account"), (void *) 0x1, CONFIG_LABEL, "", NULL, 0},
  {"", N_("%ha = host account"), (void *) 0x1, CONFIG_LABEL, "", NULL, 0},

  {"", N_("HTTP"), (void *) 0x1, CONFIG_NOTEBOOK, "", NULL, GFTP_PORT_GTK},
  {"", "", (void *) 0x1, CONFIG_TABLE, "", NULL, GFTP_PORT_GTK},
  {"http_proxy_host", N_("Proxy hostname:"), &http_proxy_host, CONFIG_CHARTEXT,
	N_("Firewall hostname"), NULL, GFTP_PORT_ALL},
  {"http_proxy_port", N_("Proxy port:"), &http_proxy_port, CONFIG_INTTEXT, 
	N_("Port to connect to on the firewall"), NULL, GFTP_PORT_ALL},
  {"http_proxy_username", N_("Proxy username:"), &http_proxy_username, 
	CONFIG_CHARTEXT, N_("Your firewall username"), NULL, GFTP_PORT_ALL},
  {"http_proxy_password", N_("Proxy password:"), &http_proxy_password, 
	CONFIG_CHARPASS, N_("Your firewall password"), NULL, GFTP_PORT_ALL},
  {"", "", (void *) 0x1, CONFIG_TABLE, "", NULL, GFTP_PORT_GTK},
  {"use_http11", N_("Use HTTP/1.1"), &use_http11, CONFIG_CHECKBOX,
	N_("Do you want to use HTTP/1.1 or HTTP/1.0"), NULL, GFTP_PORT_ALL},

  {"", N_("SSH"), (void *) 0x1, CONFIG_NOTEBOOK, "", NULL, GFTP_PORT_GTK},
  {"", "", (void *) 0x1, CONFIG_TABLE, "", NULL, GFTP_PORT_GTK},
  {"ssh_prog_name", N_("SSH Prog Name:"), &ssh_prog_name, CONFIG_CHARTEXT,
	N_("The path to the SSH executable"), NULL, GFTP_PORT_ALL},
  {"ssh_extra_params", N_("SSH Extra Params:"), &ssh_extra_params, 
        CONFIG_CHARTEXT, N_("Extra parameters to pass to the SSH program"), NULL, GFTP_PORT_ALL},
  {"ssh1_sftp_path", N_("SSH sftpserv path:"), &ssh1_sftp_path,
	CONFIG_CHARTEXT, N_("Default remote SSH sftpserv path"), NULL, GFTP_PORT_ALL},
  {"ssh2_sftp_path", N_("SSH2 sftp-server path:"), &ssh2_sftp_path,
	CONFIG_CHARTEXT, N_("Default remote SSH2 sftp-server path"), NULL, GFTP_PORT_ALL},
  {"", "", (void *) 0x1, CONFIG_TABLE, "", NULL, GFTP_PORT_GTK},
  {"ssh_need_userpass", N_("Need SSH User/Pass"), &ssh_need_userpass, CONFIG_CHECKBOX, 
        N_("Require a username/password for SSH connections"), NULL, GFTP_PORT_ALL},
  {"ssh_use_askpass", N_("Use ssh-askpass util"), &ssh_use_askpass, CONFIG_CHECKBOX, 
        N_("Use the ssh-askpass utility to grab the users password"), NULL, GFTP_PORT_GTK},
  {"sshv2_use_sftp_subsys", N_("Use SSH2 SFTP subsys"), &sshv2_use_sftp_subsys, CONFIG_CHECKBOX, 
        N_("Call ssh with the -s sftp flag. This is helpful because you won't have to know the remote path to the remote sftp-server"), NULL, GFTP_PORT_GTK},
  {"enable_old_ssh", N_("Enable old SSH protocol"), &enable_old_ssh, CONFIG_CHECKBOX, 
        N_("Enable the old SSH protocol. You will need to download the sftp server from http:///www.xbill.org/sftp"), NULL, GFTP_PORT_ALL},

  {"list_dblclk_action", "", &listbox_dblclick_action, CONFIG_HIDEINT, 
	N_("This defines what will happen when you double click a file in the file listboxes. 0=View file 1=Edit file 2=Transfer file"), NULL, 0},
  {"listbox_local_width", "", &listbox_local_width, CONFIG_HIDEINT, 
	N_("The default width of the local files listbox"), NULL, 0},
  {"listbox_remote_width", "", &listbox_remote_width, CONFIG_HIDEINT,
	N_("The default width of the remote files listbox"), NULL, 0},
  {"listbox_file_height", "", &listbox_file_height, CONFIG_HIDEINT,
	N_("The default height of the local/remote files listboxes"), NULL, 0},
  {"transfer_height", "", &transfer_height, CONFIG_HIDEINT,
	N_("The default height of the transfer listbox"), NULL, 0},
  {"log_height", "", &log_height, CONFIG_HIDEINT,
	N_("The default height of the logging window"), NULL, 0},
  {"file_trans_column", "", &file_trans_column, CONFIG_HIDEINT,
	N_("The width of the filename column in the transfer window. Set this to 0 to have this column automagically resize."), NULL, 0},
  {"local_sortcol", "", &local_sortcol, CONFIG_INTTEXT, 
	N_("The default column to sort by"), NULL, GFTP_PORT_TEXT},
  {"local_sortasds", "", &local_sortasds, CONFIG_INTTEXT,
	N_("Sort ascending or descending"), NULL, GFTP_PORT_TEXT},
  {"remote_sortcol", "", &remote_sortcol, CONFIG_INTTEXT,
	N_("The default column to sort by"), NULL, GFTP_PORT_TEXT},
  {"remote_sortasds", "", &remote_sortasds, CONFIG_INTTEXT,
	N_("Sort ascending or descending"), NULL, GFTP_PORT_TEXT},
  {"local_file_width", "", &local_columns[0], CONFIG_HIDEINT,
	N_("The width of the filename column in the file listboxes. Set this to 0 to have this column automagically resize. Set this to -1 to disable this column"), NULL, 0},
  {"local_size_width", "", &local_columns[1], CONFIG_HIDEINT,
	N_("The width of the size column in the file listboxes. Set this to 0 to have this column automagically resize. Set this to -1 to disable this column"), NULL, 0},
  {"local_user_width", "", &local_columns[2], CONFIG_HIDEINT,
	N_("The width of the user column in the file listboxes. Set this to 0 to have this column automagically resize. Set this to -1 to disable this column"), NULL, 0},
  {"local_group_width", "", &local_columns[3], CONFIG_HIDEINT,
	N_("The width of the group column in the file listboxes. Set this to 0 to have this column automagically resize. Set this to -1 to disable this column"), NULL, 0},
  {"local_date_width", "", &local_columns[4], CONFIG_HIDEINT,
	N_("The width of the date column in the file listboxes. Set this to 0 to have this column automagically resize. Set this to -1 to disable this column"), NULL, 0},
  {"local_attribs_width", "", &local_columns[5], CONFIG_HIDEINT,
	N_("The width of the attribs column in the file listboxes. Set this to 0 to have this column automagically resize. Set this to -1 to disable this column"), NULL, 0},
  {"remote_file_width", "", &remote_columns[0], CONFIG_HIDEINT,
	N_("The width of the filename column in the file listboxes. Set this to 0 to have this column automagically resize. Set this to -1 to disable this column"), NULL, 0},
  {"remote_size_width", "", &remote_columns[1], CONFIG_HIDEINT,
	N_("The width of the size column in the file listboxes. Set this to 0 to have this column automagically resize. Set this to -1 to disable this column"), NULL, 0},
  {"remote_user_width", "", &remote_columns[2], CONFIG_HIDEINT,
	N_("The width of the user column in the file listboxes. Set this to 0 to have this column automagically resize. Set this to -1 to disable this column"), NULL, 0},
  {"remote_group_width", "", &remote_columns[3], CONFIG_HIDEINT,
	N_("The width of the group column in the file listboxes. Set this to 0 to have this column automagically resize. Set this to -1 to disable this column"), NULL, 0},
  {"remote_date_width", "", &remote_columns[4], CONFIG_HIDEINT,
	N_("The width of the date column in the file listboxes. Set this to 0 to have this column automagically resize. Set this to -1 to disable this column"), NULL, 0},
  {"remote_attribs_width", "", &remote_columns[5], CONFIG_HIDEINT,
	N_("The width of the attribs column in the file listboxes. Set this to 0 to have this column automagically resize. Set this to -1 to disable this column"), NULL, 0},
  {"send_color", "", &send_color, CONFIG_COLOR,
        N_("The color of the commands that are sent to the server"), NULL, 0},
  {"recv_color", "", &recv_color, CONFIG_COLOR,
        N_("The color of the commands that are received from the server"), NULL, 0},
  {"error_color", "", &error_color, CONFIG_COLOR,
        N_("The color of the error messages"), NULL, 0},
  {"misc_color", "", &misc_color, CONFIG_COLOR,
        N_("The color of the rest of the log messages"), NULL, 0},
  {NULL, NULL, NULL, 0, "", NULL, 0}
};


void
gftp_read_config_file (char **argv, int get_xpms)
{
  char *tempstr, *temp1str, *curpos, *endpos, buf[255], *str, *red, *green, 
       *blue;
  gftp_file_extensions * tempext;
  gftp_bookmarks * newentry;
  gftp_proxy_hosts * host;
  unsigned int nums[4];
  struct hostent *hent;
  struct utsname unme;
  gftp_color * color;
  struct passwd *pw;
  FILE *conffile;
  uid_t my_uid;
  gid_t my_gid;
  int line, i;
  size_t len;
  void *ptr;

  memset (&send_color, 0, sizeof (send_color));
  memset (&recv_color, 0, sizeof (recv_color));
  memset (&error_color, 0, sizeof (error_color));
  memset (&misc_color, 0, sizeof (misc_color));
  send_color.green = 0x8600;
  recv_color.blue = 0xffff;
  error_color.red = 0xffff;
  misc_color.red = 0xffff;

  line = 0;
  bookmarks = g_malloc0 (sizeof (*bookmarks));
  bookmarks->isfolder = 1;
  bookmarks->path = g_malloc0 (1);
  bookmarks_htable = g_hash_table_new (string_hash_function, string_hash_compare);
  config_htable = g_hash_table_new (string_hash_function, string_hash_compare);

  if ((tempstr = expand_path (CONFIG_FILE)) == NULL)
    {
      printf (_("gFTP Error: Bad config file name %s\n"), CONFIG_FILE);
      exit (0);
    }

  if (access (tempstr, F_OK) == -1)
    {
      temp1str = expand_path (BASE_CONF_DIR);
      if (access (temp1str, F_OK) == -1)
	{
	  if (mkdir (temp1str, S_IRUSR | S_IWUSR | S_IXUSR) != 0)
	    {
	      printf (_("gFTP Error: Could not make directory %s: %s\n"),
		      temp1str, g_strerror (errno));
	      exit (-1);
	    }
	}
      g_free (temp1str);

      temp1str = g_strdup_printf ("%s/gftprc", SHARE_DIR);
      if (access (temp1str, F_OK) == -1)
	{
	  printf (_("gFTP Error: Cannot find master config file %s\n"),
		  temp1str);
	  printf (_("Did you do a make install?\n"));
	  exit (-1);
	}
      copyfile (temp1str, tempstr);
      g_free (temp1str);
    }

  if ((conffile = fopen (tempstr, "r")) == NULL)
    {
      printf (_("gFTP Error: Cannot open config file %s: %s\n"), CONFIG_FILE,
	      g_strerror (errno));
      exit (0);
    }
  g_free (tempstr);
 
  for (i = 0; config_file_vars[i].var != NULL; i++)
    {
      g_hash_table_insert (config_htable, config_file_vars[i].key, 
                           GINT_TO_POINTER(i));
    } 

  while (fgets (buf, sizeof (buf), conffile))
    {
      len = strlen (buf);
      if (buf[len - 1] == '\n')
	buf[--len] = '\0';
      if (len && buf[len - 1] == '\r')
	buf[--len] = '\0';
      line++;

      if (*buf == '#' || *buf == '\0')
        continue;

      if ((curpos = strchr (buf, '=')) != NULL)
        *curpos = '\0';

      if ((ptr = g_hash_table_lookup (config_htable, buf)) != NULL)
        {
          i = GPOINTER_TO_INT(ptr);
          curpos = buf + strlen (buf) + 1;
          if (config_file_vars[i].type == CONFIG_CHARTEXT ||
              config_file_vars[i].type == CONFIG_CHARPASS ||
              config_file_vars[i].type == CONFIG_TEXT)
            {
              *(char **) config_file_vars[i].var = g_malloc (strlen (curpos) + 1);
              strcpy (*(char **) config_file_vars[i].var, curpos);
            }
          else if (config_file_vars[i].type == CONFIG_FLOATTEXT)
            *(double *) config_file_vars[i].var = strtod (curpos, NULL);
          else if (config_file_vars[i].type == CONFIG_COLOR)
            {
              color = config_file_vars[i].var;
              parse_args (curpos, 3, line, &red, &green, &blue);
              color->red = strtol (red, NULL, 16);
              color->green = strtol (green, NULL, 16);
              color->blue = strtol (blue, NULL, 16);
              g_free (red);
              g_free (green);
              g_free (blue);
            }
          else
            *(int *) config_file_vars[i].var = strtol (curpos, NULL, 10);
	}
      else if (strcmp (buf, "host") == 0)
	{
          /* This is just here for compatibility with old versions of gftp */
	  curpos = buf + 5;
	  while (*curpos == '/')
	    curpos++;
	  newentry = g_malloc0 (sizeof (*newentry));
	  newentry->isfolder = 0;
	  parse_args (curpos, 8, line, &newentry->path, &newentry->hostname,
		      &temp1str, &newentry->remote_dir, &newentry->user,
		      &newentry->pass, &tempstr, &newentry->acct);
	  newentry->port = strtol (temp1str, NULL, 10);
	  g_free (temp1str);
	  newentry->save_password = *newentry->pass != '\0';
	  add_to_bookmark (newentry);
	}
      else if (strcmp (buf, "dont_use_proxy") == 0)
	{
	  curpos = buf + 15;
	  host = g_malloc0 (sizeof (*host));
	  if ((tempstr = strchr (curpos, '/')) == NULL)
	    {
	      host->domain = g_malloc (strlen (curpos) + 1);
	      strcpy (host->domain, curpos);
	    }
	  else
	    {
	      *tempstr = '\0';
	      sscanf (curpos, "%u.%u.%u.%u", &nums[0], &nums[1], &nums[2],
		      &nums[3]);
	      host->ipv4_network_address =
		nums[0] << 24 | nums[1] << 16 | nums[2] << 8 | nums[3];

	      if (strchr (tempstr + 1, '.') == NULL)
		host->ipv4_netmask =
		    0xffffffff << (32 - strtol (tempstr + 1, NULL, 10));
	      else
		{
		  sscanf (tempstr + 1, "%u.%u.%u.%u", &nums[0], &nums[1],
			  &nums[2], &nums[3]);
		  host->ipv4_netmask =
		    nums[0] << 24 | nums[1] << 16 | nums[2] << 8 | nums[3];
		}
	    }
	  proxy_hosts = g_list_append (proxy_hosts, host);
	}
      else if (strcmp (buf, "ext") == 0)
	{
          if (get_xpms)
            {
	      curpos = buf + 4;
	      tempext = g_malloc (sizeof (*tempext));
	      parse_args (curpos, 4, line, &tempext->ext, &tempext->filename,
	  	          &tempext->ascii_binary, &tempext->view_program);
 
	      if ((tempstr = get_xpm_path (tempext->filename, 1)) != NULL)
	        g_free (tempstr);

	      tempext->stlen = strlen (tempext->ext);
              registered_exts = g_list_append (registered_exts, tempext);
            }
	}
      else if (strcmp (buf, "localhistory") == 0)
	{
	  curpos = buf + 13;
	  str = g_malloc (strlen (curpos) + 1);
	  strcpy (str, curpos);
	  localhistory = g_list_append (localhistory, str);
	  localhistlen++;
	}
      else if (strcmp (buf, "remotehistory") == 0)
	{
	  curpos = buf + 14;
	  str = g_malloc (strlen (curpos) + 1);
	  strcpy (str, curpos);
	  remotehistory = g_list_append (remotehistory, str);
	  remotehistlen++;
	}
      else if (strcmp (buf, "hosthistory") == 0)
	{
	  curpos = buf + 12;
	  str = g_malloc (strlen (curpos) + 1);
	  strcpy (str, curpos);
	  host_history = g_list_append (host_history, str);
	  host_len++;
	}
      else if (strcmp (buf, "porthistory") == 0)
	{
	  curpos = buf + 12;
	  str = g_malloc (strlen (curpos) + 1);
	  strcpy (str, curpos);
	  port_history = g_list_append (port_history, str);
	  port_len++;
	}
      else if (strcmp (buf, "userhistory") == 0)
	{
	  curpos = buf + 12;
	  str = g_malloc (strlen (curpos) + 1);
	  strcpy (str, curpos);
	  user_history = g_list_append (user_history, str);
	  user_len++;
	}
      else
	{
	  printf (_("gFTP Warning: Skipping line %d in config file: %s\n"),
                  line, buf);
	}
    }

  if (default_protocol == NULL)
    {
      default_protocol = g_malloc (4);
      strcpy (default_protocol, "FTP");
    }

  if (!enable_old_ssh)
    {
      gftp_protocols[GFTP_SSH_NUM].name = NULL;
      gftp_protocols[GFTP_SSH_NUM].init = NULL;
      gftp_protocols[GFTP_SSH_NUM].url_prefix = NULL;
      gftp_protocols[GFTP_SSH_NUM].shown = 0;
    }

  if ((tempstr = expand_path (LOG_FILE)) == NULL)
    {
      printf (_("gFTP Error: Bad log file name %s\n"), LOG_FILE);
      exit (0);
    }

  if ((logfd = fopen (tempstr, "w")) == NULL)
    {
      printf (_("gFTP Warning: Cannot open %s for writing: %s\n"),
              tempstr, g_strerror (errno));
    }
  g_free (tempstr);

  gftp_read_bookmarks ();

  if (ssh_extra_params != NULL)
    {
      num_ssh_extra_params = 0;
      curpos = ssh_extra_params;
      while ((endpos = strchr (curpos, ' ')) != NULL)
        {
          *endpos = '\0';
          num_ssh_extra_params++;
          ssh_extra_params_list = g_realloc (ssh_extra_params_list,
                                             (num_ssh_extra_params + 1) * 
                                             sizeof (char *));
          ssh_extra_params_list[num_ssh_extra_params - 1] = g_strdup (curpos);
          ssh_extra_params_list[num_ssh_extra_params] = NULL;
          *endpos = ' ';
          curpos = endpos + 1;
        }

      if (*curpos != '\0')
        {
          num_ssh_extra_params++;
          ssh_extra_params_list = g_realloc (ssh_extra_params_list,
                                             (num_ssh_extra_params + 1) * 
                                             sizeof (char *));
          ssh_extra_params_list[num_ssh_extra_params - 1] = g_strdup (curpos);
          ssh_extra_params_list[num_ssh_extra_params] = NULL;
        }
    }

  make_nonnull (&proxy_config);
  make_nonnull (&firewall_host);
  make_nonnull (&firewall_username);
  make_nonnull (&firewall_password);
  make_nonnull (&firewall_account);
  make_nonnull (&http_proxy_host);
  make_nonnull (&http_proxy_username);
  make_nonnull (&http_proxy_password);
  make_nonnull (&view_program);
  make_nonnull (&edit_program);

  my_uid = geteuid ();
  my_gid = getegid ();
  pw = getpwuid (my_uid);
  if (emailaddr == NULL || *emailaddr == '\0')
    {
      /* If there is no email address specified, then we'll just use the
         currentuser@currenthost */
      if (emailaddr)
        g_free (emailaddr);
      uname (&unme);
      hent = gethostbyname (unme.nodename);
      if (strchr (unme.nodename, '.') == NULL && hent != NULL)
        emailaddr = g_strconcat (pw->pw_name, "@", hent->h_name, NULL);
      else
        emailaddr = g_strconcat (pw->pw_name, "@", unme.nodename, NULL);
      gftp_write_config_file ();
    }
}


void
gftp_read_bookmarks (void)
{
  char *tempstr, *temp1str, buf[255], *curpos;
  gftp_bookmarks *newentry;
  FILE *bmfile;
  size_t len;
  int line;

  if ((tempstr = expand_path (BOOKMARKS_FILE)) == NULL)
    {
      printf (_("gFTP Error: Bad bookmarks file name %s\n"), BOOKMARKS_FILE);
      exit (0);
    }

  if (access (tempstr, F_OK) == -1)
    {
      temp1str = g_strdup_printf ("%s/bookmarks", SHARE_DIR);
      if (access (temp1str, F_OK) == -1)
	{
	  printf (_("Warning: Cannot find master bookmark file %s\n"),
		  temp1str);
	  g_free (temp1str);
	  return;
	}
      copyfile (temp1str, tempstr);
      g_free (temp1str);
    }

  if ((bmfile = fopen (tempstr, "r")) == NULL)
    {
      printf (_("gFTP Error: Cannot open bookmarks file %s: %s\n"), tempstr,
	      g_strerror (errno));
      exit (0);
    }
  g_free (tempstr);

  line = 0;
  newentry = NULL;
  while (fgets (buf, sizeof (buf), bmfile))
    {
     len = strlen (buf);
      if (buf[len - 1] == '\n')
	buf[--len] = '\0';
      if (len && buf[len - 1] == '\r')
	buf[--len] = '\0';
      line++;

      if (*buf == '[')
	{
	  newentry = g_malloc0 (sizeof (*newentry));
	  for (; buf[len - 1] == ' ' || buf[len - 1] == ']'; buf[--len] = '\0');
	  newentry->path = g_malloc (len);
	  strcpy (newentry->path, buf + 1);
	  newentry->isfolder = 0;
	  add_to_bookmark (newentry);
	}
      else if (strncmp (buf, "hostname", 8) == 0 && newentry)
	{
	  curpos = buf + 9;
	  if (newentry->hostname)
	    g_free (newentry->hostname);
	  newentry->hostname = g_malloc (strlen (curpos) + 1);
	  strcpy (newentry->hostname, curpos);
	}
      else if (strncmp (buf, "port", 4) == 0 && newentry)
	newentry->port = strtol (buf + 5, NULL, 10);
      else if (strncmp (buf, "protocol", 8) == 0 && newentry)
	{
	  curpos = buf + 9;
	  if (newentry->protocol)
	    g_free (newentry->protocol);
	  newentry->protocol = g_malloc (strlen (curpos) + 1);
	  strcpy (newentry->protocol, curpos);
	}
      else if (strncmp (buf, "remote directory", 16) == 0 && newentry)
	{
	  curpos = buf + 17;
	  if (newentry->remote_dir)
	    g_free (newentry->remote_dir);
	  newentry->remote_dir = g_malloc (strlen (curpos) + 1);
	  strcpy (newentry->remote_dir, curpos);
	}
      else if (strncmp (buf, "local directory", 15) == 0 && newentry)
	{
	  curpos = buf + 16;
	  if (newentry->local_dir)
	    g_free (newentry->local_dir);
	  newentry->local_dir = g_malloc (strlen (curpos) + 1);
	  strcpy (newentry->local_dir, curpos);
	}
      else if (strncmp (buf, "username", 8) == 0 && newentry)
	{
	  curpos = buf + 9;
	  if (newentry->user)
	    g_free (newentry->user);
	  newentry->user = g_malloc (strlen (curpos) + 1);
	  strcpy (newentry->user, curpos);
	}
      else if (strncmp (buf, "password", 8) == 0 && newentry)
	{
	  curpos = buf + 9;
	  if (newentry->pass)
	    g_free (newentry->pass);
	  newentry->pass = g_malloc (strlen (curpos) + 1);
	  strcpy (newentry->pass, curpos);
	  newentry->save_password = *newentry->pass != '\0';
	}
      else if (strncmp (buf, "account", 7) == 0 && newentry)
	{
	  curpos = buf + 8;
	  if (newentry->acct)
	    g_free (newentry->acct);
	  newentry->acct = g_malloc (strlen (curpos) + 1);
	  strcpy (newentry->acct, curpos);
	}
      else if (strncmp (buf, "sftpserv_path", 13) == 0 && newentry)
        {
          curpos = buf + 14;
          if (newentry->sftpserv_path)
            g_free (newentry->sftpserv_path);
          newentry->sftpserv_path = g_malloc (strlen (curpos) + 1);
          strcpy (newentry->sftpserv_path, curpos);
        }
      else if (*buf != '#' && *buf != '\0')
	printf (_("gFTP Warning: Skipping line %d in bookmarks file: %s\n"),
		line, buf);
    }
}


void
add_to_bookmark (gftp_bookmarks * newentry)
{
  gftp_bookmarks *preventry, *folderentry, *endentry;
  char *curpos;

  if (!newentry->protocol)
    {
      newentry->protocol = g_malloc (4);
      strcpy (newentry->protocol, "FTP");
    }

  /* We have to create the folders. For example, if we have 
     Debian Sites/Debian, we have to create a Debian Sites entry */
  preventry = bookmarks;
  if (preventry->children != NULL)
    {
      endentry = preventry->children;
      while (endentry->next != NULL)
	endentry = endentry->next;
    }
  else
    endentry = NULL;
  curpos = newentry->path;
  while ((curpos = strchr (curpos, '/')) != NULL)
    {
      *curpos = '\0';
      /* See if we already made this folder */
      if ((folderentry = (gftp_bookmarks *)
	   g_hash_table_lookup (bookmarks_htable, newentry->path)) == NULL)
	{
	  /* Allocate the individual folder. We have to do this for the edit 
	     bookmarks feature */
	  folderentry = g_malloc0 (sizeof (*folderentry));
	  folderentry->path = g_malloc (strlen (newentry->path) + 1);
	  strcpy (folderentry->path, newentry->path);
	  folderentry->prev = preventry;
	  folderentry->isfolder = 1;
	  g_hash_table_insert (bookmarks_htable, folderentry->path,
			       folderentry);
	  if (preventry->children == NULL)
	    preventry->children = folderentry;
	  else
	    endentry->next = folderentry;
	  preventry = folderentry;
	  endentry = NULL;
	}
      else
	{
	  preventry = folderentry;
	  if (preventry->children != NULL)
	    {
	      endentry = preventry->children;
	      while (endentry->next != NULL)
		endentry = endentry->next;
	    }
	  else
	    endentry = NULL;
	}
      *curpos = '/';
      curpos++;
    }

  /* Get the parent node */
  if ((curpos = strrchr (newentry->path, '/')) == NULL)
    preventry = bookmarks;
  else
    {
      *curpos = '\0';
      preventry = (gftp_bookmarks *)
	g_hash_table_lookup (bookmarks_htable, newentry->path);
      *curpos = '/';
    }

  if (preventry->children != NULL)
    {
      endentry = preventry->children;
      while (endentry->next != NULL)
	endentry = endentry->next;
      endentry->next = newentry;
    }
  else
    preventry->children = newentry;
  newentry->prev = preventry;
  newentry->next = NULL;
  g_hash_table_insert (bookmarks_htable, newentry->path, newentry);
}


void
gftp_write_config_file (void)
{
  char *hdr, *proxyhdr, *exthdr, *histhdr;
  gftp_file_extensions *tempext;
  gftp_proxy_hosts *hosts;
  gftp_color *color;
  GList *templist;
  FILE *conffile;
  char *tempstr;
  int pos;

  hdr = N_("Config file for gFTP. Copyright (C) 1998-2002 Brian Masney <masneyb@gftp.org>. Warning: Any comments that you add to this file WILL be overwritten. If a entry has a (*) in it's comment, you can't change it inside gFTP");
  proxyhdr = N_("This section specifies which hosts are on the local subnet and won't need to go out the proxy server (if available). Syntax: dont_use_proxy=.domain or dont_use_proxy=network number/netmask");
  exthdr = N_("ext=file extenstion:XPM file:Ascii or Binary (A or B):viewer program. Note: All arguments except the file extension are optional");
  histhdr = N_("This section contains the data that is in the history");

  if ((tempstr = expand_path (CONFIG_FILE)) == NULL)
    {
      printf (_("gFTP Error: Bad config file name %s\n"), CONFIG_FILE);
      exit (0);
    }

  if ((conffile = fopen (tempstr, "w+")) == NULL)
    {
      printf (_("gFTP Error: Cannot open config file %s: %s\n"), CONFIG_FILE,
	      g_strerror (errno));
      exit (0);
    }

  g_free (tempstr);

  write_comment (conffile, _(hdr));
  fwrite ("\n", 1, 1, conffile);

  for (pos = 0; config_file_vars[pos].var != NULL; pos++)
    {
      if (config_file_vars[pos].type == CONFIG_CHARTEXT ||
          config_file_vars[pos].type == CONFIG_CHARPASS ||
          config_file_vars[pos].type == CONFIG_TEXT)
        {
          if (*config_file_vars[pos].comment != '\0')
            write_comment (conffile, _(config_file_vars[pos].comment));

	  fprintf (conffile, "%s=%s\n\n", config_file_vars[pos].key,
	  	   *(char **) config_file_vars[pos].var == NULL ? "" :
                   *(char **) config_file_vars[pos].var);
        }
      else if (config_file_vars[pos].type == CONFIG_FLOATTEXT)
        {
          if (*config_file_vars[pos].comment != '\0')
            write_comment (conffile, _(config_file_vars[pos].comment));

	  fprintf (conffile, "%s=%.2f\n\n", config_file_vars[pos].key,
                   *(double *) config_file_vars[pos].var);
        }
      else if (config_file_vars[pos].type == CONFIG_INTTEXT ||
               config_file_vars[pos].type == CONFIG_UINTTEXT ||
               config_file_vars[pos].type == CONFIG_CHECKBOX ||
               config_file_vars[pos].type == CONFIG_HIDEINT)
        {
          if (*config_file_vars[pos].comment != '\0')
            write_comment (conffile, _(config_file_vars[pos].comment));

	  fprintf (conffile, "%s=%d\n\n", config_file_vars[pos].key,
                   *(int *) config_file_vars[pos].var);
        }
      else if (config_file_vars[pos].type == CONFIG_COLOR)
        {
          if (*config_file_vars[pos].comment != '\0')
            write_comment (conffile, _(config_file_vars[pos].comment));

          color = config_file_vars[pos].var;
          fprintf (conffile, "%s=%x:%x:%x\n\n", config_file_vars[pos].key,
                   color->red, color->green, color->blue);
        }
    }

  write_comment (conffile, _(proxyhdr));
  templist = proxy_hosts;
  while (templist != NULL)
    {
      hosts = templist->data;
      if (hosts->domain)
	fprintf (conffile, "dont_use_proxy=%s\n", hosts->domain);
      else
	fprintf (conffile, "dont_use_proxy=%d.%d.%d.%d/%d.%d.%d.%d\n",
		 hosts->ipv4_network_address >> 24 & 0xff,
		 hosts->ipv4_network_address >> 16 & 0xff,
		 hosts->ipv4_network_address >> 8 & 0xff,
		 hosts->ipv4_network_address & 0xff,
		 hosts->ipv4_netmask >> 24 & 0xff,
		 hosts->ipv4_netmask >> 16 & 0xff,
		 hosts->ipv4_netmask >> 8 & 0xff, hosts->ipv4_netmask & 0xff);
      templist = templist->next;
    }

  fwrite ("\n", 1, 1, conffile);
  write_comment (conffile, _(exthdr));
  templist = registered_exts;
  while (templist != NULL)
    {
      tempext = templist->data;
      fprintf (conffile, "ext=%s:%s:%c:%s\n", tempext->ext, tempext->filename,
	       *tempext->ascii_binary == '\0' ? ' ' : *tempext->ascii_binary,
	       tempext->view_program);
      templist = templist->next;
    }

  fwrite ("\n", 1, 1, conffile);
  write_comment (conffile, _(histhdr));
  for (templist = localhistory; templist != NULL; templist = templist->next)
    fprintf (conffile, "localhistory=%s\n", (char *) templist->data);

  for (templist = remotehistory; templist != NULL; templist = templist->next)
    fprintf (conffile, "remotehistory=%s\n", (char *) templist->data);

  for (templist = host_history; templist != NULL; templist = templist->next)
    fprintf (conffile, "hosthistory=%s\n", (char *) templist->data);

  for (templist = port_history; templist != NULL; templist = templist->next)
    fprintf (conffile, "porthistory=%s\n", (char *) templist->data);

  for (templist = user_history; templist != NULL; templist = templist->next)
    fprintf (conffile, "userhistory=%s\n", (char *) templist->data);

  fclose (conffile);
}


void
gftp_write_bookmarks_file (void)
{
  gftp_bookmarks *tempentry;
  char *bmhdr, *tempstr;
  FILE * bmfile;

  bmhdr = N_("Bookmarks file for gFTP. Copyright (C) 1998-2002 Brian Masney <masneyb@gftp.org>. Warning: Any comments that you add to this file WILL be overwritten");

  if ((tempstr = expand_path (BOOKMARKS_FILE)) == NULL)
    {
      printf (_("gFTP Error: Bad bookmarks file name %s\n"), CONFIG_FILE);
      exit (0);
    }

  if ((bmfile = fopen (tempstr, "w+")) == NULL)
    {
      printf (_("gFTP Error: Cannot open bookmarks file %s: %s\n"),
	      CONFIG_FILE, g_strerror (errno));
      exit (0);
    }

  g_free (tempstr);

  write_comment (bmfile, _(bmhdr));
  fwrite ("\n", 1, 1, bmfile);

  tempentry = bookmarks->children;
  while (tempentry != NULL)
    {
      if (tempentry->children != NULL)
	{
	  tempentry = tempentry->children;
	  continue;
	}
      tempstr = tempentry->path;
      while (*tempstr == '/')
	tempstr++;
      fprintf (bmfile,
	       "[%s]\nhostname=%s\nport=%d\nprotocol=%s\nremote directory=%s\nlocal directory=%s\nusername=%s\npassword=%s\naccount=%s\n",
	       tempstr, tempentry->hostname == NULL ? "" : tempentry->hostname,
	       tempentry->port, tempentry->protocol == NULL
	       || *tempentry->protocol ==
	       '\0' ? gftp_protocols[0].name : tempentry->protocol,
	       tempentry->remote_dir == NULL ? "" : tempentry->remote_dir,
	       tempentry->local_dir == NULL ? "" : tempentry->local_dir,
	       tempentry->user == NULL ? "" : tempentry->user,
	       !tempentry->save_password
	       || tempentry->pass == NULL ? "" : tempentry->pass,
	       tempentry->acct == NULL ? "" : tempentry->acct);

      if (tempentry->sftpserv_path)
        fprintf (bmfile, "sftpserv_path=%s\n", tempentry->sftpserv_path);

      fprintf (bmfile, "\n");
 
      if (tempentry->next == NULL)
	{
	  tempentry = tempentry->prev;
	  while (tempentry->next == NULL && tempentry->prev != NULL)
	    tempentry = tempentry->prev;
	  tempentry = tempentry->next;
	}
      else
	tempentry = tempentry->next;
    }

  fclose (bmfile);
}


static void
write_comment (FILE * fd, const char *comment)
{
  const char *pos, *endpos;

  fwrite ("# ", 1, 2, fd);
  pos = comment;
  while (strlen (pos) > 76)
    {
      for (endpos = pos + 76; *endpos != ' ' && endpos > pos; endpos--);
      if (endpos == pos)
	{
	  for (endpos = pos + 76; *endpos != ' ' && *endpos != '\0';
	       endpos++);
	}
      fwrite (pos, 1, endpos - pos, fd);
      fwrite ("\n# ", 1, 3, fd);
      if (*endpos == '\0')
	{
	  pos = endpos;
	  break;
	}
      else
	pos = endpos + 1;
    }
  if (strlen (pos) > 1)
    {
      fwrite (pos, 1, strlen (pos), fd);
      fwrite ("\n", 1, 1, fd);
    }
}


static int
parse_args (char *str, int numargs, int lineno, char **first, ...)
{
  char *curpos, *endpos, *pos, **dest, tempchar;
  int ret, has_colon;
  va_list argp;

  ret = 1;
  va_start (argp, first);
  curpos = str;
  dest = first;
  *dest = NULL;
  while (numargs > 0)
    {
      has_colon = 0;
      if (numargs > 1)
	{
	  if ((endpos = strchr (curpos, ':')) == NULL)
	    {
	      printf (_("gFTP Warning: Line %d doesn't have enough arguments\n"), 
                      lineno);
	      ret = 0;
	      endpos = curpos + strlen (curpos);
	    }
	  else
	    {
	      /* Allow colons inside the fields. If you want a colon inside a 
                 field, just put 2 colons in there */
	      while (endpos != NULL && *(endpos - 1) == '\\')
		{
		  endpos = strchr (endpos + 1, ':');
		  has_colon = 1;
		}
	    }
	}
      else
	endpos = curpos + strlen (curpos);

      *dest = g_malloc (endpos - curpos + 1);
      tempchar = *endpos;
      *endpos = '\0';
      strcpy (*dest, curpos);
      *endpos = tempchar;
      if (has_colon)
	{
	  pos = *dest;
	  curpos = *dest;
	  while (*pos != '\0')
	    {
	      if (*pos != '\\' && *(pos + 1) != ':')
		*curpos++ = *pos++;
	      else
		pos++;
	    }
	  *curpos = '\0';
	}
      if (*endpos == '\0')
	break;
      curpos = endpos + 1;
      if (numargs > 1)
	{
	  dest = va_arg (argp, char **);
	  *dest = NULL;
	}
      numargs--;
    }

  while (numargs > 1)
    {
      dest = va_arg (argp, char **);
      *dest = g_malloc (1);
      **dest = '\0';
      numargs--;
    }
  va_end (argp);
  return (1);
}


GHashTable *
build_bookmarks_hash_table (gftp_bookmarks * entry)
{
  gftp_bookmarks * tempentry;
  GHashTable * htable;

  htable = g_hash_table_new (string_hash_function, string_hash_compare);
  tempentry = entry;
  while (tempentry != NULL)
    {
      g_hash_table_insert (htable, tempentry->path, tempentry);
      if (tempentry->children != NULL)
	{
	  tempentry = tempentry->children;
	  continue;
	}
      while (tempentry->next == NULL && tempentry->prev != NULL)
	tempentry = tempentry->prev;
      tempentry = tempentry->next;
    }
  return (htable);
}


void
print_bookmarks (gftp_bookmarks * bookmarks)
{
  gftp_bookmarks * tempentry;

  tempentry = bookmarks->children;
  while (tempentry != NULL)
    {
      printf ("Path: %s (%d)\n", tempentry->path, tempentry->children != NULL);
      if (tempentry->children != NULL)
        {
          tempentry = tempentry->children;
          continue;
        }

      if (tempentry->next == NULL)
        {
	  while (tempentry->next == NULL && tempentry->prev != NULL)
            tempentry = tempentry->prev;
          tempentry = tempentry->next;
        }
      else
	tempentry = tempentry->next;
    }
}

