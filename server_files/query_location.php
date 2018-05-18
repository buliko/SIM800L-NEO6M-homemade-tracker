<?php
   class MyDB extends SQLite3 {
      function __construct() {
         $this->open('gps.db');
      }
   }
   
   $db = new MyDB();
   if(!$db) {
      echo $db->lastErrorMsg();
   } else {
      echo "Opened database successfully\n";
	  	  echo "<br>";
   }

   $sql =<<<EOF
      SELECT * FROM locations;
EOF;

   $ret = $db->query($sql);
   while($row = $ret->fetchArray(SQLITE3_ASSOC) ) {
      echo "id = ". $row['id'];
	  echo "time = ". $row['time'] . "\t";
      echo "lat = ". $row['lat'] ."\t";
      echo "lon = ". $row['lon'] ."\t";
	  echo "<br>";
   }
   echo "Operation done successfully\n";
   $db->close();
?>