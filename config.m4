dnl $Id$
dnl config.m4 for extension ip2country

PHP_ARG_ENABLE(ip2country, whether to enable ip2country support,
[  --enable-ip2country     Enable ip2country support])

if test "$PHP_IP2COUNTRY" != "no"; then

  PHP_NEW_EXTENSION(ip2country, ip2country.c, $ext_shared)
fi
