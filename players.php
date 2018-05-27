<?php
$database = $_GET["database"];

	$connstring = 'dbname=k' . $database . ' user=k2';
	$conn = pg_connect($connstring) or die ("could not connect");
	$query = 'select * from k2s.player order by rating desc';
	$rs = pg_query($conn, $query);

	while ($row = pg_fetch_row($rs)) {
		echo json_encode($row);
	}
	pg_close($conn);
?>
