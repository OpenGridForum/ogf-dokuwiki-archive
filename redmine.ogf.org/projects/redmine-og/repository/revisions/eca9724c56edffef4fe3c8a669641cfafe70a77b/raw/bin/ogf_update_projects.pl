#!/usr/bin/perl -w

# mysql -u redmine -p -e 'SELECT * from projects' redmine

BEGIN {
  use strict;
  use DBI;
}

my $HOST = 'redmine.ogf.org';
my $DATA = 'redmine';
my $USER = 'redmine';
my $PASS = '064adm1n';
my $TYPE = 'mysql';

my $i = 0;
{
  # Connect to the database.
  my $dsn = "DBI:$TYPE:database=$DATA;host=$HOST";
  my $dbh = DBI->connect ($dsn, "$USER", "$PASS", {'RaiseError' => 1});

  # retrieve data from the projects.
  my $sth = $dbh->prepare ("SELECT * FROM projects");
  $sth->execute ();
  
  while ( my $ref = $sth->fetchrow_hashref () ) 
  {
    print "Found a row: id = $ref->{'id'}, name = $ref->{'name'}\n";
  }

  # free results
  $sth->finish ();

  # Disconnect from the database.
  $dbh->disconnect ();
}



END {
}

