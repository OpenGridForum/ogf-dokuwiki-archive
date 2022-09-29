#!/usr/bin/perl -w

BEGIN {
  use strict;
}

my $map       = "./projects.map";
my $conv_docs = "./forge2redmine_documents.pl";

######################################################################
#
# main
#
{
  open        (IN, "<$map") or die "Cannot open $map: $!\n";
  my @lines = <IN>;
  close       (IN);

  chomp (@lines);

  LINE:
  foreach my $line ( @lines )
  {
    $line =~ s/#.*//iog;

    if ( $line =~ /^\s*$/io )
    {
      # ignore empty lines
    }
    elsif ( $line =~ /^\s*(\S.*?)\s*:\s*(\S.*?)\s*$/io )
    {
      my $src = $1;
      my $tgt = $2;

      if ( $src eq '-' )
      {
        next LINE;
      }
      
      print   "$conv_docs \t'$src' \t'$tgt'\n";
      system ("$conv_docs \t'$src' \t'$tgt'");
    }
    else
    {
      die "Cannot handle $line\n";
    }
  }
}



