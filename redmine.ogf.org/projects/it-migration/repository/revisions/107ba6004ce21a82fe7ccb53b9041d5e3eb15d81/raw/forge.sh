#!/bin/sh

# cat forge.psql.dump.gz | gunzip > forge.psql.dump

sudo apt-get --yes --force-yes purge   postgresql-8.4
sudo apt-get --yes --force-yes install postgresql-8.4

echo "CREATE DATABASE gridforge_db;"                                  | sudo -u postgres psql
echo "CREATE USER     gridforge_user;"                                | sudo -u postgres psql
echo "ALTER  USER     gridforge_user WITH PASSWORD 'gridforge_user';" | sudo -u postgres psql
echo "ALTER  USER     postgres       WITH PASSWORD '$PASS';"          | sudo -u postgres psql

sudo psql -U gridforge_user -w -h localhost -f forge.psql.dump -d gridforge_db

# rm forge.psql.dump

