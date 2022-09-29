#!/usr/bin/perl -w


BEGIN {
  use strict;

  use DBI;
  use Data::Dumper;

  sub inspect_folder ($$$);
  sub handle_doc     ($$$);
  sub rm_mkdir       ($);
  sub dump_sql       ($);
}

my $DEBUG          = 0;

my $forge_project  = shift || die "\n\t$0 <forge-project-id> <redmine-project-id>\n\n";
my $rmine_project  = shift || die "\n\t$0 <forge-project-id> <redmine-project-id>\n\n";

   $forge_project  =~ s/'/\\'/g;
   $rmine_project  =~ s/'/\\'/g;

my $forge_pid      = undef;
my $rmine_pid      = undef;

my $forge          = undef;
my $rmine          = undef;

my $rmine_fallback = 'migration-archive';
my $prefix         = 'gridforge';
my $rmine_root     = "/var/www/redmine/files/dmsf";

my $forge_db_host  = 'forge.ogf.org';
my $forge_db_port  = '5432';
my $forge_db_name  = 'gridforge_db';
my $forge_db_user  = 'gridforge_user';
my $forge_db_pass  = $ENV{'FORGE_DB_PASS'};

my $rmine_db_host  = 'localhost';
my $rmine_db_port  = '3306';
my $rmine_db_name  = 'redmine';
my $rmine_db_user  = 'redmine';
my $rmine_db_pass  = $ENV{'RMINE_DB_PASS'};
my $rmine_user     = 'www-data';
my $rmine_group    = 'www-data';

if ( $rmine_project eq '-' )
{
  $rmine_project  = $rmine_fallback;
  $prefix         = lc ($forge_project);
  $prefix         =~ s/[^a-z0-9\.]+/-/g;
  $prefix         = "gridforge/$prefix";
}

my $sql           = undef;

# print "  --- $forge_project \t -> $rmine_project \t (/$prefix/)\n";
# exit (0);

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
  
  $sql = qq{SELECT * FROM projects WHERE identifier = '$rmine_project'};
  dump_sql ($sql);
  my $rquery = $rmine->prepare ($sql);
     $rquery->execute();
  
  if ( $rquery->rows () != 1 ) {
    die "Could not uniquely identify redmine project\n";
  }
  else {
    $rmine_pid = $rquery->fetchrow_hashref ()->{'id'};
  }
  undef($rquery);


  $forge = DBI->connect ("dbi:Pg:dbname=$forge_db_name;host=$forge_db_host;port=$forge_db_port", 
                         $forge_db_user, $forge_db_pass,
                        {AutoCommit => 0, 
                         RaiseError => 1, 
                         PrintError => 1});
  
  $sql = qq{SELECT * FROM project WHERE title = '$forge_project'};
  dump_sql ($sql);
  $fquery = $forge->prepare ($sql);
  $fquery->execute();
  
  if ( $fquery->rows () != 1 ) {
    die "Could not uniquely identify forge project\n";
  }
  else {
    my $frow      = $fquery->fetchrow_hashref ();
    my $folder    = $frow->{'root_folder_id'};
       $forge_pid = $frow->{'id'};

    print "\n";
    print "  gridforge project: $forge_project [$forge_pid]\n";
    print "  redmine   project: $rmine_project [$rmine_pid]\n";
    print "\n";

    if ( ! $folder )
    {
      print "  no root folder found on forge\n";
      exit (0);
    }

    inspect_folder ($folder, "Project Root", "")
  }
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
    $sql = qq{SELECT * FROM item WHERE folder_id = '$f_id'};
    dump_sql ($sql);
    my $query = $forge->prepare ($sql);
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
    $sql = qq{SELECT * FROM folder WHERE parent_folder_id = '$f_id'};
    dump_sql ($sql);
    my $query = $forge->prepare ($sql);
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

     $d_title =~ s/'/\\'/g; 

  $d_path =~ s!^/Project Root/Documents/Root Folder!!og;
  $d_path =  "/$prefix$d_path";
  print "  $d_path : $d_title [$d_id]\n";

  my $folder_id = rm_mkdir ($d_path);

  $sql = qq{SELECT * FROM document WHERE id = '$d_id'};
  dump_sql ($sql);
  my $iquery = $forge->prepare ($sql);
     $iquery->execute();
  
  ITEM:
  while ( my $irow = $iquery->fetchrow_hashref () )
  {
    my $d_des = $irow->{'description'};
    my $v_id  = $irow->{'latest_version_id'};

    # ignore documents we already have
    $sql = qq{SELECT * FROM dmsf_files WHERE name           = '$d_title' 
                                         AND project_id     = $rmine_pid 
                                         AND dmsf_folder_id = $folder_id};
    dump_sql ($sql);
    my $tquery = $rmine->prepare ($sql);
       $tquery->execute();

    if ( $tquery->rows () != 0 )
    {
      # ignore this document item
    # print "                        ignore document $rmine_pid : $folder_id : $d_title\n";
      print "                        ignore\n";
      my $trow = $tquery->fetchrow_hashref ();
      $rd_id   = $trow->{'id'};
    }
    else
    {
      # new document, insert

      my $d_insert = qq{
        INSERT INTO dmsf_files (id,   project_id, dmsf_folder_id, name,       deleted, created_at, updated_at)
        VALUES                 (NULL, $rmine_pid, $folder_id,     '$d_title', 0      , NOW(),        NOW());
      };
      
      dump_sql ($d_insert); 
      $rmine->do ($d_insert) or die "Cannot create document: $d_path/$d_title: " . $rmine->errstr . "\n";
      $rd_id = $rmine->last_insert_id (undef, undef, 'dmsf_files', 'id');
    }

    $sql = qq{SELECT * FROM document_version WHERE document_id = '$d_id'};
    dump_sql ($sql);
    my $vquery = $forge->prepare ($sql);
       $vquery->execute();

    VERSION:
    while ( my $vrow = $vquery->fetchrow_hashref () )
    {
      my $v_comment = $vrow->{'version_comment'};
      my $v_version = $vrow->{'version'};
      my $v_date    = $vrow->{'date_created'};
      my $v_status  = $vrow->{'status'};
      my $f_id      = $vrow->{'stored_file_id'};

         $v_comment =~ s/'/\\'/g;

      my $frow = {};

      if ( defined ($f_id) )
      {
        $sql = qq{SELECT * FROM stored_file WHERE id = '$f_id'};
        dump_sql ($sql);
        my $fquery = $forge->prepare ($sql);
           $fquery->execute();
  
        if ( $fquery->rows () != 1 )
        {
          die "Could not uniquely identify version sourcefile\n";
        }
        
        $frow = $fquery->fetchrow_hashref ();
      }

      my $f_raw  = $frow->{'raw_file_id'} || undef;
      my $f_name = $frow->{'file_name'}   || "";
      my $f_mime = $frow->{'mime_type'}   || "none";
      my $f_size = $frow->{'file_size'}   || 0;
      my $f_del  = $frow->{'is_deleted'}  || 1;

         $f_name =~ s/'/\\'/g;
  
      my $tgt_r  = "$rmine_root";
      my $tgt_d  = "$rmine_project$d_path";
      my $tgt_f  = "$f_name";

      # if ( $tgt_f =~ /^(.*)\.([^\.]{1,4}?)$/ )
      # {
      #   $tgt_f  = "$1.v$v_version.$2";
      # }
      # else
      # {
      #   $tgt_f  = "v$v_version\_$tgt_f";
      # }

      my $tgt   = "$tgt_r/$tgt_d/$tgt_f";

      if ( defined ($f_raw) )
      {
        my $s1    = substr ($f_raw, 0, 1);
        my $s2    = substr ($f_raw, 0, 2);
        my $s3    = substr ($f_raw, 0, 3);
        my $s4    = substr ($f_raw, 0, 4);
        my $src_d = "/data/sourceforge_var/filestorage/";

        my $src   = "$src_d/$s1/$s2/$s3/$s4/$f_raw";

        print "                 -- $v_version : $d_title [$f_name] : [$f_raw] - $f_mime - $f_size" . "b ($v_comment)\n";
        print "                        amerzky\@forge.ogf.org:$src  -->  $tgt\n";

        if ( ! -e $tgt )
        {
          qx{mkdir -p                          "$tgt_r/$tgt_d/"};
          qx{scp "amerzky\@forge.ogf.org:$src" "$tgt_r/$tgt_d/$tgt_f"};
          qx{chown -R $rmine_user:$rmine_group "$tgt_r/"};
        }
      }

      # ignore revisions we already have
      $sql = qq{SELECT * FROM dmsf_file_revisions WHERE name           = '$f_name' 
                                                    AND title          = '$d_title' 
                                                    AND project_id     = $rmine_pid 
                                                    AND dmsf_file_id   = $rd_id 
                                                    AND dmsf_folder_id = $folder_id 
                                                    AND major_version  = $v_version};
      dump_sql ($sql);
      my $tquery = $rmine->prepare ($sql);
         $tquery->execute();

      if ( $tquery->rows () != 0 )
      {
        # ignore this revision item
        print "                        ignore\n";
      }
      else
      {
        # new document, insert
        my $v_insert = qq{
          INSERT INTO dmsf_file_revisions 
            (id,                  project_id,           dmsf_file_id,       name,     
             dmsf_folder_id,      disk_filename,        size,               mime_type, 
             title,               description,          major_version,      minor_version, 
             comment,             deleted,              user_id,            created_at, updated_at)
          VALUES                                              
            (NULL,                $rmine_pid,           $rd_id,             '$f_name', 
            $folder_id,           '$tgt_d/$tgt_f',      $f_size,            '$f_mime', 
            '$d_title',           '$v_comment\n',       $v_version,         0,             
            NULL,                 $f_del,               1,                  '$v_date', '$v_date');
        };
        
        dump_sql ($v_insert); 
        # FIXME: check if exists
        $rmine->do ($v_insert) or die "Cannot create document version: $d_path/$d_title : $v_version : " . $rmine->errstr . "\n";
        my $rv_id = $rmine->last_insert_id (undef, undef, 'dmsf_file_revisions', 'id');
      }
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

  $sql = qq{SELECT * FROM dmsf_folders WHERE project_id = $rmine_pid};
  dump_sql ($sql);
  my $query = $rmine->prepare ($sql);
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
      INSERT INTO dmsf_folders (id,   project_id,   dmsf_folder_id, title, user_id, created_at, updated_at)
      VALUES                   (NULL, $rmine_pid, $parent_id, '$r_elem', 1      , NOW(),        NOW());
    };

    dump_sql ($insert); 
    $rmine->do ($insert) or die "Cannot create $r_elem: " . $rmine->errstr . "\n";
    $parent_id = $rmine->last_insert_id (undef, undef, 'dmsf_folders', 'id');
  }

  return $parent_id;
}


sub dump_sql ($)
{
  my $sql = shift;

  if ( $DEBUG )
  {
    print "                        == $sql\n";
  }
}

