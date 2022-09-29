#!/usr/bin/perl -w

# mysql -u redmine -p -e 'SELECT * from projects' redmine

BEGIN {
  use strict;
  use DBI;
}

my $HOST = 'localhost';
my $DATA = 'redmine';
my $USER = 'redmine';
my $PASS = '064admin';
my $TYPE = 'mysql';

my $i = 0;
{
  # Connect to the database.
  printf "%d\n", $i++;
  my $dsn = "DBI:$TYPE:database=$DATA;host=$HOST";
  printf "%d\n", $i++;
  my $dbh = DBI->connect ($dsn, "$USER", "$PASS", {'RaiseError' => 1});
  printf "%d\n", $i++;

  # retrieve data from the projects.
  my $sth = $dbh->prepare ("SELECT * FROM projects");
  printf "%d\n", $i++;
  $sth->execute ();
  printf "%d\n", $i++;
  
  while ( my $ref = $sth->fetchrow_hashref () ) 
  {
  printf "%d\n", $i++;
    print "Found a row: id = $ref->{'id'}, name = $ref->{'name'}\n";
  }

  # free results
  printf "%d\n", $i++;
  $sth->finish ();

  # Disconnect from the database.
  printf "%d\n", $i++;
  $dbh->disconnect ();
  printf "%d\n", $i++;
}



END {
}

