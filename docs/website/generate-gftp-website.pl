#!/usr/bin/perl -w

# This script will generate the gFTP webpage. You will have to first build the 
# RPMs, DEBs, and tarballs and put them all in the current directory.

use Time::Local;

my @date = localtime (time ());

my $version = `cat ../../configure | grep ^VERSION= | awk -F= '{print \$2}'`;
chop ($version);

my %rep = ("STABLE_BZ2" => "gftp-" . $version . ".tar.bz2",
           "STABLE_GZ" => "gftp-" . $version . ".tar.gz",
           "STABLE_I386RPM" => "gftp-" . $version . "-1.i386.rpm",
           "STABLE_SRCRPM" => "gftp-" . $version . "-1.src.rpm",
           "STABLE_I386DEB" => "gftp_" . $version . "-1_i386.deb",
           "STABLE_I386DEB_COMMON" => "gftp-common_" . $version . "-1_i386.deb",
           "STABLE_I386DEB_GTK" => "gftp-gtk_" . $version . "-1_i386.deb",
           "STABLE_I386DEB_TEXT" => "gftp-text_" . $version . "-1_i386.deb"); 

$tarfiles = "MD5SUMS changelog.html gftp-screenshot.png index.html logo.jpg readme.html robots.txt screenshots.html";

print "Generating MD5SUMS...\n";

open M, ">MD5SUMS" || die "Can't open MD5SUMS: $!\n";
foreach $tag (keys %rep)
  {
    next if $tag =~ /_KB$/;

    $file = $rep{$tag};
    if (!-e $file)
      {
        undef ($rep{$tag});
        next;
      }

    $tarfiles .= " $file";
    print M `md5sum $file`;

    @st = stat ($file);
    $rep{$tag . "_KB"} = int ($st[7] / 1024);
  }
close M;

$rep{"STABLE_VER"} = $version;
$rep{"STABLE_DATE"} = ++$date[4] . "/$date[3]/" . ($date[5] + 1900);

print "Generating changelog.html...\n";

open C, "<../../ChangeLog-old" or die "Can't open ../../ChangeLog-old: $!\n";
open N, ">changelog.html" or die "Can't open changelog.html: $!\n";

print N "<HTML>\n";
print N "<HEAD>\n";
print N "<TITLE>gFTP Changelog</TITLE>\n";
print N "<META NAME=\"author\" CONTENT=\"Brian Masney\">\n";
print N "<LINK REV=MADE HREF=\"mailto:masneyb\@gftp.org\">\n";
print N "</HEAD>\n";

print N "<BODY BGCOLOR=\"#FFFFFF\" TEXT=\"#000000\" LINK=\"#336699\" VLINK=\"#336699\" ALINK=\"#336699\">\n";
print N "<FONT FACE=\"Lucida,Verdana,Helvetica,Arial\"><SMALL>\n";

$close = 0;

while (<C>)
  {
    chop;
    next if /^\s*$/;

    if (/^Changes/)
      {
        if ($close)
          {
            print N "\n</UL>\n";
          }
        else
          {
            $close = 1;
          }  
     
        print N "\n<H4>$_</H4>\n";
        print N "<UL>";
        next;
      }

    s/\</&lt\;/g;
    s/\>/&gt\;/g;
    s/\s+$//;

    if (!/^\*/)
      {
        s/^\s+//;
        print N " $_";
      }
    else
      {
        s/^\*\s+//;
        print N "\n<LI>$_";
      }
  }

print N "\n</UL>\n\n";
print N "<P>Brian Masney <A HREF=\"mailto:masneyb\@gftp.org\">masneyb\@gftp.org</A></P>\n";
print N "</SMALL></FONT></BODY></HTML>\n";

close C;
close N;

print "Generating index.html...\n";

open I, "<index.html.in" || die "Can't open index.html.in: $!\n";
open N, ">index.html" || die "Can't open index.html: $!\n";
while (<I>)
  {
    $skip = 0;
    while (($var) = /\%(.*?)\%/)
      {
        if (!defined ($rep{$var}))
          {
            print STDERR "Warning: $var not found in hash. Skipping.\n";
            $skip = 1;
            last;
          }
        s/\%$var\%/$rep{$var}/g;
      }

    next if $skip == 1;
    print N $_;
  }
close I;
close N;

$cmd = "cp ../gftp-faq/gftp-faq.html readme.html";
print "$cmd\n";
system ($cmd);

$tarcmd = "tar -jcvf gftp-$version-website.tar.bz2 $tarfiles";
print "Running $tarcmd...\n";
system ($tarcmd);

print "Done.\n";

