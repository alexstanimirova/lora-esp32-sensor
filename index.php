<?php

// set timezone
date_default_timezone_set('Europe/Sofia');

// read json file
$json = file_get_contents('data.json');
  
// decode json
$data = json_decode($json, true);

$temperature = $data['temperature'][1];
$humidity = $data['humidity'][1];
$ts = $data['ts'];

?>

<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Helium Sensor Demo</title>
</head>
<body>
    <div>
        <strong>Temperature:</strong>
        <span><?php echo number_format($temperature, 2); ?> °C</span>
    </div>
    <div>
        <strong>Humidity:</strong>
        <span><?php echo number_format($humidity, 2); ?> %</span>
    </div>
    <div>
        <strong>Updated at:</strong>
        <span><?php echo date('Y-m-d H:i:s', $ts); ?></span>
    </div>

    <script>
        setTimeout(function(){
            window.location.reload(1);
        }, 60 * 1000);
    </script>
</body>
</html>