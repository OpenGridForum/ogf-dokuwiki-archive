#!/usr/bin/perl -w


BEGIN {
  use strict;

  use DBI;
  use Data::Dumper;

  sub inspect_folder ($$$);
  sub handle_doc     ($$$);
  sub rm_mkdir       ($);
}

my $forge_project = shift || die "\n\t$0 <forge-project-id> <redmine-project-id>\n\n";
my $rmine_project = shift || die "\n\t$0 <forge-project-id> <redmine-project-id>\n\n";

my $forge_pid = undef;
my $rmine_pid = undef;

my $forge     = undef;
my $rmine     = undef;

######################################################################
#
# main
#
{
  print "$forge_project\n";

  $rmine = DBI->connect ('dbi:mysql:dbname=redmine;host=localhost;port=3306', 
                         'redmine', $ENV{'PASS'},
                        {AutoCommit => 1, 
                         RaiseError => 1, 
                         PrintError => 1});
  
  my $rquery = $rmine->prepare ("SELECT * FROM projects WHERE identifier = '$rmine_project'");
     $rquery->execute();
  
  if ( $rquery->rows () != 1 )
  {
    die "Could not uniquely identify redmine project\n";
  }
  else
  {
    $rmine_pid = $rquery->fetchrow_hashref ()->{'id'};
  }

  print "redmine project id: $rmine_pid\n";

  $forge = DBI->connect ('dbi:Pg:dbname=gridforge_db;host=localhost;port=5432', 
                         'postgres', $ENV{'PASS'},
                        {AutoCommit => 0, 
                         RaiseError => 1, 
                         PrintError => 1});
  
  $fquery = $forge->prepare ("SELECT * FROM project WHERE title = '$forge_project'");
  $fquery->execute();
  
  if ( $fquery->rows () != 1 )
  {
    die "Could not uniquely identify forge project\n";
  }
  else
  {
    my $frow      = $fquery->fetchrow_hashref ();
    my $folder    = $frow->{'root_folder_id'};
       $forge_pid = $frow->{'id'};

    inspect_folder ($folder, "Project Root", "")
  }

  undef($rquery);
  undef($fquery);

  $forge->disconnect ();
  $rmine->disconnect ();
}

######################################################################
#
sub inspect_folder ($$$)
{
  my $f_id    = shift;
  my $f_title = shift;
  my $path    = shift;

  # print "  $path/$f_title [$f_id]\n";

  if ( $path =~ m/^\/Project\s+Root\/.+/o &&
       $path !~ m/^\/Project\s+Root\/Documents/o )
  {
    return;
  }

  # print " ---> $path [$f_id]\n";

  # handle items in folder
  {
    my $query = $forge->prepare ("SELECT * FROM item WHERE folder_id = '$f_id'");
       $query->execute();
  
    while ( my $row = $query->fetchrow_hashref () )
    {
      my $i_id      = $row->{'id'};
      my $i_title   = $row->{'title'};
      my $i_deleted = $row->{'is_deleted'};

      if ( ! $i_deleted )
      {
        if ( $i_id =~ /^doc\d+$/o )
        {
          handle_doc ($i_id, $i_title, "$path/$f_title");
        }
      }
    }
  }


  # handle sub-folders
  {
    my $query = $forge->prepare ("SELECT * FROM folder WHERE parent_folder_id = '$f_id'");
       $query->execute();
  
    while ( my $row = $query->fetchrow_hashref () )
    {
      my $sf_id    = $row->{'id'};
      my $sf_title = $row->{'title'};

      inspect_folder ($sf_id, $sf_title, "$path/$f_title");
    }
  }
}

######################################################################
#
sub handle_doc ($$$)
{
  my $d_id    = shift;
  my $d_title = shift;
  my $d_path  = shift;

  $d_path =~ s!^/Project Root/Documents/Root Folder/!/!og;
  $d_path =  "/gridforge$d_path";
  print "  $d_path : $d_title [$d_id]\n";

  my $folder_id = rm_mkdir ($d_path);

  my $iquery = $forge->prepare ("SELECT * FROM document WHERE id = '$d_id'");
     $iquery->execute();
  
  while ( my $irow = $iquery->fetchrow_hashref () )
  {
    my $d_des = $irow->{'description'};
    my $v_id  = $irow->{'latest_version_id'};

    my $d_insert = qq{
      INSERT INTO redmine.dmsf_files (id,   project_id, dmsf_folder_id, name,       deleted, created_at, updated_at)
      VALUES                         (NULL, $rmine_pid, $folder_id,     '$d_title', 0      , NOW(),        NOW());
    };
    
    # print " $d_insert\n"; 
    # FIXME: check if exists
    $rmine->do ($d_insert) or die "Cannot create document: $d_path/$d_title: " . $rmine->errstr . "\n";
    $rd_id = $rmine->last_insert_id (undef, undef, 'dmsf_files', 'id');
    # print " ==========> d $rd_id\n";

    my $vquery = $forge->prepare ("SELECT * FROM document_version WHERE document_id = '$d_id'");
       $vquery->execute();

    while ( my $vrow = $vquery->fetchrow_hashref () )
    {
      my $v_comment = $vrow->{'version_comment'};
      my $v_version = $vrow->{'version'};
      my $v_date    = $vrow->{'date_created'};
      my $v_status  = $vrow->{'status'};
      my $f_id      = $vrow->{'stored_file_id'};


      my $fquery = $forge->prepare ("SELECT * FROM stored_file WHERE id = '$f_id'");
         $fquery->execute();
  
      if ( $fquery->rows () != 1 )
      {
        die "Could not uniquely identify version sourcefile\n";
      }
      
      $frow = $fquery->fetchrow_hashref ();

      my $f_raw  = $frow->{'raw_file_id'};
      my $f_name = $frow->{'file_name'};
      my $f_mime = $frow->{'mime_type'};
      my $f_size = $frow->{'file_size'};
      my $f_del  = $frow->{'is_deleted'};

      my $tgt_d = "/var/www/redmine/files/dmsf/$rmine_project$d_path/";
      my $tgt   = "$tgt_d/$f_name";

      if ( $tgt =~ /^(.*)\.([^\.]{1,4}?)$/ )
      {
        $tgt = "$1.v$v_version.$2";
      }
      else
      {
        $tgt = "v$v_version\_$tgt";
      }

      my $s1    = substr ($f_raw, 0, 1);
      my $s2    = substr ($f_raw, 0, 2);
      my $s3    = substr ($f_raw, 0, 3);
      my $s4    = substr ($f_raw, 0, 4);
      my $src_d = "/data/sourceforge_var/filestorage/";
      my $src   = "$src_d/$s1/$s2/$s3/$s4/$f_raw";


      print "                 -- $v_version : $d_title [$f_name] : [$f_raw] - $f_mime - $f_size" . "b ($v_comment)\n";
      print "                        amerzky\@forge.ogf.org:$src  -->  $tgt\n";

      qx{mkdir -p '$tgt_d'};
      qx{scp 'amerzky\@forge.ogf.org:$src' '$tgt'};
  
      my $v_insert = qq{
        INSERT INTO redmine.dmsf_file_revisions 
          (id,                  project_id,           dmsf_file_id,       name,     
           dmsf_folder_id,      disk_filename,        size,               mime_type, 
           title,               description,          major_version,      minor_version, 
           comment,             deleted,              user_id,            created_at, updated_at)
        VALUES                                              
          (NULL,                $rmine_pid,           $rd_id,             '$f_name', 
          $folder_id,           '$tgt',               $f_size,            '$f_mime', 
          '$d_title',           '$v_comment\n',       $v_version,         0,             
          NULL,                 $f_del,               1,                  '$v_date', '$v_date');
      };
      
      # print " $v_insert\n"; 
      # FIXME: check if exists
      $rmine->do ($v_insert) or die "Cannot create document version: $d_path/$d_title : $v_version : " . $rmine->errstr . "\n";
      my $rv_id = $rmine->last_insert_id (undef, undef, 'dmsf_file_revisions', 'id');
      # print " ==========> v $rv_id\n";
    }
  }

  print "\n";
  
}


######################################################################
#
sub rm_mkdir ($)
{
  my $path  = shift;
     $path  =~ s!^/*!!og;

  # print " ~~~ $path\n";

  my @elems = split (/\//, $path);

  my $query = $rmine->prepare ("SELECT * FROM dmsf_folders WHERE project_id = '$rmine_pid'");
     $query->execute();

  my %folders = ();
  
  while ( my $row = $query->fetchrow_hashref () )
  {
    my $id = $row->{'id'};

    $folders{$id}{'name'  } = $row->{'title'};
    $folders{$id}{'desc'  } = $row->{'description'};
    $folders{$id}{'parent'} = $row->{'dmsf_folder_id'} || "NULL";
  }

  # print Dumper \$path;
  # print Dumper \%folders;
  # print Dumper \@elems;

  my $parent_id = "NULL";
  my $cwd       = "";

  ELEM:
  foreach my $elem ( @elems )
  {
    my $found = 0;

    foreach my $id ( keys (%folders) )
    {
      if ( $folders{$id}{'parent'} eq $parent_id &&
           $folders{$id}{'name'  } eq $elem      )
      {
        $parent_id = $id;
        $found     = 1;
      }
    }

    if ( ! $found )
    {
      last ELEM;
    }

    $cwd = $cwd .= "/$elem";
  }

  # we may have found some part of the path, and need to create the remainder
  # under the last vald parent_id
  my $remainder = "/$path";
     $remainder =~ s!^$cwd/*!!g;

  if ( $parent_id ne "NULL" ) { $parent_id = "'$parent_id'"; }

  my @r_elems   = split (/\/+/, $remainder);
  my $last_elem = "";

  for my $r_elem ( @r_elems )
  {
    my $insert = qq{
      INSERT INTO redmine.dmsf_folders (id,   project_id,   dmsf_folder_id, title, user_id, created_at, updated_at)
      VALUES                           (NULL, $rmine_pid, $parent_id, '$r_elem', 1      , NOW(),        NOW());
    };

    # print "do $insert\n";
    $rmine->do ($insert) or die "Cannot create $r_elem: " . $rmine->errstr . "\n";

    $parent_id = $rmine->last_insert_id (undef, undef, 'dmsf_folders', 'id');
    # print " ==========> p $parent_id\n";
  }

  return $parent_id;
}



