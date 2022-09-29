#!/bin/sh

cat redmine.mysql.dump.gz | gunzip > redmine.mysql.dump

sudo apt-get --yes --force-yes purge   mysql-server
sudo apt-get --yes --force-yes install mysql-server

sudo chmod -R 755 /var/lib/mysql/
sudo chmod -R 755 /var/run/mysqld/


sudo /etc/init.d/mysql stop
sudo mysqld --skip-grant-tables &
sleep 3


echo "UPDATE user SET Password=PASSWORD('$RMINE_DB_PASS') WHERE User='root';" | sudo mysql -u root mysql
echo "FLUSH PRIVILEGES; "                                                     | sudo mysql -u root -p$RMINE_DB_PASS mysql

sleep 1
sudo /etc/init.d/mysql stop
sleep 1
killall    mysqld
sleep 1
killall -9 mysqld

sudo chmod -R 755 /var/lib/mysql/
sudo chmod -R 755 /var/run/mysqld/
sudo /etc/init.d/mysql start

echo "CREATE USER 'redmine'@'localhost' IDENTIFIED BY '$RMINE_DB_PASS'; "      | sudo mysql -u root -p$RMINE_DB_PASS mysql
echo "CREATE USER 'redmine'@'%'         IDENTIFIED BY '$RMINE_DB_PASS'; "      | sudo mysql -u root -p$RMINE_DB_PASS mysql

echo "GRANT ALL PRIVILEGES ON *.* TO 'redmine'@'localhost' WITH GRANT OPTION; " | sudo mysql -u root -p$RMINE_DB_PASS mysql 
echo "GRANT ALL PRIVILEGES ON *.* TO 'redmine'@'%'         WITH GRANT OPTION; " | sudo mysql -u root -p$RMINE_DB_PASS mysql

echo "FLUSH PRIVILEGES; "                                                       | sudo mysql -u root -p$RMINE_DB_PASS mysql

sudo mysql -u redmine -p$RMINE_DB_PASS redmine < redmine.mysql.dump

rm redmine.mysql.dump

