#!/usr/bin/perl -w


BEGIN {
  use strict;

  use DBI;
  use Data::Dumper;

  sub inspect_folder ($;$);
  sub dump_sql       ($);
}


my $DEBUG          = 0;

my $rmine          = undef;
my $prefix         = 'gridforge';
my $rmine_root     = "/var/www/redmine/files/dmsf";

my $rmine_db_host  = 'localhost';
my $rmine_db_port  = '3306';
my $rmine_db_name  = 'redmine';
my $rmine_db_user  = 'redmine';
my $rmine_db_pass  = $ENV{'RMINE_DB_PASS'};
my $rmine_user     = 'www-data';
my $rmine_group    = 'www-data';

my $sql            = undef;

######################################################################
#
# main
#
{
  $rmine = DBI->connect ("dbi:mysql:dbname=$rmine_db_name;host=$rmine_db_host;port=$rmine_db_port", 
                         $rmine_db_user, $rmine_db_pass,
                        {AutoCommit => 1, 
                         RaiseError => 1, 
                         PrintError => 1});
  
  my $sql = qq{SELECT * FROM dmsf_folders WHERE title = 'gridforge'};
  dump_sql ($sql);
  my $query = $rmine->prepare ($sql);
     $query->execute();

  while ( my $row = $query->fetchrow_hashref () )
  {
    my $id = $row->{'id'};

    print "gridforge [$id]\n";

    inspect_folder ($id);
  }


  $rmine->disconnect ();
}

######################################################################
#
sub inspect_folder ($;$)
{
  my $id    = shift;
  my $ind    = shift || 2;


  {
    my $dsql = qq{SELECT * FROM dmsf_files WHERE dmsf_folder_id = $id};
    dump_sql ($dsql);
    my $dquery = $rmine->prepare ($dsql);
       $dquery->execute();

    while ( my $drow = $dquery->fetchrow_hashref () )
    {
      my $dname = $drow->{'name'};
      my $did   = $drow->{'id'};

      print " " x $ind;
      print "$dname [$did]\n";

      my $Rsql = qq{DELETE FROM dmsf_file_revisions WHERE dmsf_file_id = $did};
      dump_sql ($Rsql);
      $rmine->do ($Rsql) or die "Cannot remove revisions : " . $rmine->errstr . "\n";
    }

    my $Dsql = qq{DELETE FROM dmsf_files WHERE dmsf_folder_id = $id};
    dump_sql ($Dsql);
    $rmine->do ($Dsql) or die "Cannot remove revisions : " . $rmine->errstr . "\n";
  }


  {
    my $fsql = qq{SELECT * FROM dmsf_folders WHERE dmsf_folder_id = $id};
    dump_sql ($fsql);
    my $fquery = $rmine->prepare ($fsql);
       $fquery->execute();

    while ( my $frow = $fquery->fetchrow_hashref () )
    {
      my $ftitle = $frow->{'title'};
      my $fid    = $frow->{'id'};

      print " " x $ind;
      print "$ftitle/ [$fid]\n";

      inspect_folder ($fid, $ind + 2);
    }

    my $Fsql = qq{DELETE FROM dmsf_folders WHERE dmsf_folder_id = $id};
    dump_sql ($Fsql);
    $rmine->do ($Fsql) or die "Cannot remove revisions : " . $rmine->errstr . "\n";
  }
}


######################################################################
#
sub dump_sql ($)
{
  my $dsql = shift;

  if ( $DEBUG )
  {
    print "                        == $dsql\n";
  }
}

