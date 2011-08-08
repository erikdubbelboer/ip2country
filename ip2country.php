<!DOCTYPE html>
<html lang=nl>
<head>
<meta charset=utf-8>
<title>ip2country</title>
</head>
<body>
<?php

if (extension_loaded('ip2country')) {
  $stat = ip2country_stat();

  echo "<h1>Statistics</h1>";

  /* ip2country_stat will return NULL when statistics are disabled */
  if (!is_array($stat)) {
    echo '<p>statistics are disabled in the ip2country.ini file (setting name is ip2country.stat).</p>';
  } else {
    /* convert the last misses from numers to ip address strings */
    foreach ($stat['lastmisses'] as &$ip) {
      $ip = long2ip($ip);
    }
    unset($ip); /* always unset the iterator after an & loop */

    $total = $stat['hits'] + $stat['misses'];

    /* no division by zero kk tnx */
    if ($total > 0) {
      $stat['misspercentage'] = number_format($stat['misses'] / $total * 100, 2).'%';
    }

    echo '<pre>';
    print_r($stat);
    echo '</pre>';
  }


  $ip = $_SERVER['REMOTE_ADDR'];
  if (isset($_GET['ip'])) {
    $ip = $_GET['ip'];
  }

  echo '<h2>Looking up ',$ip,'</h2>';

  echo '<pre>';
  var_dump(ip2country($ip, true));
  echo '</pre>';

  ?>
  <form method=get>
  <p>
  <label>IP <input type=text name=ip /> </label>
  </p>
  <p>
  <input type=submit value="Perform lookup">
  </p>
  </form>
  <?php
} else {
  echo '<p>the ip2country extension is not loaded.</p>';
}
?>
</body>
</html>

