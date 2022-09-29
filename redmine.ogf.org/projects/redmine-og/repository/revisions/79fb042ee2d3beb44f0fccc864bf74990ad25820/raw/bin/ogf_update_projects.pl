#!/usr/bin/perl -w

# mysql -u redmine -p -e 'SELECT * from projects' redmine

BEGIN {
  use strict;
  use DBI;
  use Data::Dumper;
}

my $HOST   = 'localhost';
my $DATA   = 'redmine';
my $USER   = 'redmine';
my $PASS   = '064admin';
my $TYPE   = 'mysql';

my $ROOT   = '/var/www/redmine';
my $DETAIL = "$ROOT/doc/details";

my $INDEX  = '<iframe width="100%" height="2000" scrolling="no" frameborder="no" src="http://www.ogf.org/mailman/listinfo/###LIST###"></iframe>';

my $MLIST_FID = undef;

my $i = 0;
{
  # Connect to the database.
  my $dsn = "DBI:$TYPE:database=$DATA;host=$HOST";
  my $dbh = DBI->connect ($dsn, "$USER", "$PASS", {'RaiseError' => 1});

  # retrieve data from the custom_fields, to get field id for 'mailinglist'
  # project field
  my $cfields_query = $dbh->prepare ("SELECT * FROM custom_fields");
     $cfields_query->execute ();
  while ( my $ref = $cfields_query->fetchrow_hashref () ) 
  {
    if ( $ref->{'name'} =~ /^mailing\s*list.*$/i )
    {
      $MLIST_FID = $ref->{'id'};
    }
  }
  
  # retrieve data from the projects.
  my $projects_query = $dbh->prepare ("SELECT * FROM projects");
     $projects_query->execute ();
  
  PROW:
  while ( my $ref = $projects_query->fetchrow_hashref () ) 
  {
    my $id   = $ref->{'id'}         || die "no id?\n";
    my $name = $ref->{'name'}       || die "no name?\n";
    my $acro = $ref->{'identifier'} || die "no identifier?\n";
    my $list = $acro; # fall back to use the group id

    # if ( $acro ne 'nml-wg' )
    # {
    #   next PROW;
    # }

    # check if a mailing list is definecd for this project.  Pull that from the
    # 'custom_values' table
    my $cvalues_query = $dbh->prepare ("SELECT * FROM custom_values");
       $cvalues_query->execute ();
    while ( my $ref = $cvalues_query->fetchrow_hashref () ) 
    {
      if ( $ref->{'customized_type'} eq 'Project'  &&
           $ref->{'customized_id'}   eq $id        &&
           $ref->{'custom_field_id'} eq $MLIST_FID &&
           $ref->{'value'}           !~ /^\s*$/o   )
      {
        $list = $ref->{'value'};
      }
    }

    if ( $acro =~ /-[wrco]g$/o )
    {
      
      if ( ! -d "$DETAIL/$acro" )
      {
				print "mkdir $DETAIL/$acro\n";
        mkdir "$DETAIL/$acro";
      }

      # if ( ! -e "$DETAIL/$acro/index.html" )
      {
				print "create $DETAIL/$acro/index.html \t ($name : $list\@ogf.org)\n";

        my $tmp = $INDEX;
        $tmp =~ s/###LIST###/$list/g;

        # print "$list\n";
        # print "$tmp\n";
        open  (IDX, ">$DETAIL/$acro/index.html") || die "Cannot create $DETAIL/$acro/index.html: $!\n";
        print  IDX  "\n  $tmp\n\n";
        close (IDX);
      }
    }
  }

  # free results
  $projects_query->finish ();

  # Disconnect from the database.
  $dbh->disconnect ();
}



END {
}

