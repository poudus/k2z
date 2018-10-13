<?php
$database = $_GET['database'];
$channel = $_GET['channel'];

	$connstring = 'dbname=k' . $database . ' user=k2';
	$conn = pg_connect($connstring) or die ("could not connect");
	$query = 'update k2s.live set vp = 999, vpid = 9999 where channel = ' . $channel . ' and vp = 0 and vpid = 0';
	$rs = pg_query($conn, $query);

	http_response_code(200);
?>
