<?php
$database = $_GET['database'];
$channel = $_GET['channel'];
$move = $_GET['move'];

	$connstring = 'dbname=k' . $database . ' user=k2';
	$conn = pg_connect($connstring) or die ("could not connect");
	$query = "update k2s.live set trait = 'H', length = length + 1, last_move = '" . $move . "', moves = CONCAT(moves,'" . $move . "') where channel = " . $channel . " and vp = 999 and vpid = 9999";
	$rs = pg_query($conn, $query);

	http_response_code(200);
?>
