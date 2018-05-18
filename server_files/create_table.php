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
   }

 

    
   $sql =<<<EOF
      CREATE TABLE locations
      (id INTEGER PRIMARY KEY AUTOINCREMENT, device_id INTEGER NOT NULL,
      lat           TEXT    NOT NULL,
      lon           TEXT     NOT NULL, time TEXT NOT NULL);
EOF;

   $ret = $db->exec($sql);
   if(!$ret){
      echo $db->lastErrorMsg();
   } else {
      echo "Table created successfully\n";
   }
   $db->close();
?>
