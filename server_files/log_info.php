<?php

	$db = new SQLite3('gps.db');
	
	echo "Database object is created</br>";
	
	$current_server_time = date("H:i:s");
	$device_id = $_GET['dev_id'];
	$lat = $_GET['lat'];
	$lon = $_GET['lon'];
	
	echo "Current server time set</br>";
	
	$statement = $db->prepare('INSERT INTO "locations" ("device_id", "lat", "lon", "time") VALUES (:dev_id, :lat, :lon, :time)');
	
	$statement->bindValue(':dev_id', $device_id);
	$statement->bindValue(':lat', $lat);
	$statement->bindValue(':lon', $lon);
	$statement->bindValue(':time', $current_server_time);
	
	$statement->execute();
	
	echo "Query statement is executed (INSERT)";
	
?>