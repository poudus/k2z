<?php
$database = $_GET["database"];
$channel = $_GET["channel"];

	$connstring = 'dbname=k' . $database . ' user=k2';
	$conn = pg_connect($connstring) or die ("could not connect");
	$query = 'select channel, signature, last_move, length from k2s.live where channel = ' . $channel;
	$rs = pg_query($conn, $query);

	http_response_code(200);

	while ($row = pg_fetch_row($rs)) {
		echo json_encode($row);
	}
	pg_close($conn);

?>
