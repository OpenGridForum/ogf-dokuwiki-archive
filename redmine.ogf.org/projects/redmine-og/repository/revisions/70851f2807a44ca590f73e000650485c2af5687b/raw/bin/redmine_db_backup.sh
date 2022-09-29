#!/bin/sh

bak=/var/www/redmine/backup
tgt="$bak/redmine_`date '+%Y_%m_%d'`.gz"

/usr/bin/mysqldump -u root -p064admin redmine | gzip > $tgt

mv $bak/*01.gz $bak/monthly > /dev/null 2>&1 || true


