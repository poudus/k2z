<?php
$max = $_GET["max"];
echo $max;

	$connstring = 'dbname=kdb12 user=k2';
	$conn = pg_connect($connstring) or die ("could not connect");
echo $conn;
	$query = 'select * from k2s.game order by ts desc';
	$rs = pg_query($conn, $query);

	while ($row = pg_fetch_row($rs)) {
		echo json_encode($row);
	}
	pg_close($conn);
?>
