#!/bin/sh

cat redmine.mysql.dump.gz | gunzip > redmine.mysql.dump

sudo apt-get --yes --force-yes purge   mysql-server
sudo apt-get --yes --force-yes install mysql-server

sudo chmod -R 755 /var/lib/mysql/
sudo chmod -R 755 /var/run/mysqld/


sudo /etc/init.d/mysql stop
sudo mysqld --skip-grant-tables &
sleep 3


echo "UPDATE user SET Password=PASSWORD('$PASS') WHERE User='root';" | sudo mysql -u root mysql

echo "FLUSH PRIVILEGES; " | sudo mysql -u root -p$PASS mysql

sleep 1
killall -9 mysqld

sudo /etc/init.d/mysql stop
sudo chmod -R 755 /var/lib/mysql/
sudo chmod -R 755 /var/run/mysqld/
sudo /etc/init.d/mysql start

echo "CREATE USER 'redmine'@'localhost' IDENTIFIED BY '$PASS'; "                | sudo mysql -u root -p$PASS mysql
echo "CREATE USER 'redmine'@'%'         IDENTIFIED BY '$PASS'; "                | sudo mysql -u root -p$PASS mysql

echo "GRANT ALL PRIVILEGES ON *.* TO 'redmine'@'localhost' WITH GRANT OPTION; " | sudo mysql -u root -p$PASS mysql 
echo "GRANT ALL PRIVILEGES ON *.* TO 'redmine'@'%'         WITH GRANT OPTION; " | sudo mysql -u root -p$PASS mysql

echo "FLUSH PRIVILEGES; "                                                       | sudo mysql -u root -p$PASS mysql

sudo mysql -u redmine -p$PASS redmine < redmine.mysql.dump

rm redmine.mysql.dump

