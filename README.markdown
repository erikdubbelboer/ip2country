ip2country
==========

ip2country module for PHP.

You can send comments, patches, questions [here on github](https://github.com/ErikDubbelboer/ip2country/issues) or to erik@mininova.org.


Installing/Configuring
======================

To build the database, run:

<pre>
cd makedb
make
wget http://www.maxmind.com/download/geoip/database/GeoIPCountryCSV.zip
unzip GeoIPCountryCSV.zip
./makedb GeoIPCountryWhois.csv /var/www/geoip.db
</pre>

Where `/var/www/geoip.db` is the location where you want to store your database file.


To install the module, run:

<pre>
phpize
./configure CFLAGS="-O3"
make
sudo make install
</pre>

You will need to edit the `ip2country.ini` file to point to your extension and database file. You can find your extension directory using:

<pre>
php-config --extension-dir
</pre>

Now copy `ip2country.ini` to your php install's configuration directory. On Gentoo this will usually be `/etc/php/cgi-php5/ext-active/`. On Ubuntu this will be `/etc/php5/conf.d`

<pre>
cp ip2country.ini /etc/php5/conf.d/
</pre>

Usage
=====

The module introduces 3 new functions to php.

<pre>
mixed ip2country(mixed ip [, bool fullname])
string code2country(string code)
array ip2country_stat()
</pre>

You can use the ip2country function to get the country code for a specific ip address. If fullname is set to true an array containing both the code and name is returned.

<pre>
$code = ip2country($_SERVER['REMOTE_ADDR']);
</pre>

The code2country function can be used to look up the full name of a country.

<pre>
$name = code2country($code);
</pre>

