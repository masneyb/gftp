#!/usr/bin/perl -w

# Brian Masney <masneyb@gftp.org>

my ($host, $user, $pass, $account, $descr, %bmhash);
use strict;

open NRC, "<.netrc" or die "Can't open .netrc: $!\n";
open BM, "+>>.gftp/bookmarks" or die "Can't open .gftp/bookmarks: $!\nTry running gFTP once to create a default bookmarks file\n";
seek (BM, 0, 0);
while (<BM>)
  {
    ($descr) = /\[(.*?)\]/;
    next if !defined ($descr);
    $bmhash{$descr} = 1;
  }

seek (BM, 0, 2);

while (<NRC>)
  {
    if (/machine /)
      {
        print_bookmark ();
        ($host) = /machine (.*?)\s+/;
      }

    if (/login /)
      {
        ($user) = /login (.*?)\s+/;
      }

    if (/password /)
      {
        ($pass) = /password (.*?)\s+/;
      }

    if (/account /)
      {
        ($account) = /account (.*?)\s+/;
      }
  }

print_bookmark ();

close NRC;
close BM;

print "The contents of your .netrc file should now be stored in .gftp/bookmarks\n";


sub print_bookmark
{
  my $i;

  return if !defined ($host);
 
  if (!defined ($bmhash{$host}))
    { $descr = $host; }
  else
    {
      for ($i=0; ; $i++)
        {
          $descr = "$host ($i)";
          last if !defined ($bmhash{$descr});
        }
    }
  

  print BM "[$descr]\n";
  print BM "hostname=$host\n";
  print BM "port=21\n";
  print BM "protocol=FTP\n";
  print BM "remote directory=\n";
  print BM "local directory=\n";
  if (!defined ($user))
    { $user = "anonymous"; }
  print BM "username=$user\n";
  if ($user eq "anonymous" || !defined ($pass))
    { $pass = "\@EMAIL\@"; }
  print BM "password=$pass\n";
  if (!defined ($account))
    { $account = ""; }
  print BM "account=$account\n\n";

  print "Added $descr = $user\@$host\n";

  undef ($host);
  undef ($user);
  undef ($pass);
  undef ($account);
}

