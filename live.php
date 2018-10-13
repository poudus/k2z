<?php
$database = $_GET["database"];
$channel = $_GET["channel"];

	$connstring = 'dbname=k' . $database . ' user=k2';
	$conn = pg_connect($connstring) or die ("could not connect");
	$query = 'select lc.channel, lc.signature, lc.last_move, lc.length, lc.trait, lc.winner, lc.reason, hp.name, vp.name from k2s.live lc, k2s.player hp, k2s.player vp where lc.hp = hp.id and lc.vp = vp.id and channel = ' . $channel;
	$rs = pg_query($conn, $query);

	http_response_code(200);

	while ($row = pg_fetch_row($rs)) {
		echo json_encode($row);
	}
	pg_close($conn);

?>
