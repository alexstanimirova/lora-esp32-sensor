<?php

if (!array_key_exists('HTTP_TOKEN', $_SERVER) || $_SERVER['HTTP_TOKEN'] != 'RZ31p8YXjIk1k87OZCtJ') {
    echo 'No token provided.';
    exit();
}

if ($_SERVER['REQUEST_METHOD'] != 'POST') {
    echo 'Only POST method is supported.';
    exit();
}

// get json
$json = file_get_contents('php://input');

if (!$json) {
    echo 'Request body is required.';
    exit();
}

// parse json
$data = json_decode($json);

if (!property_exists($data, 'payload')) {
    echo 'Payload property is required in body.';
    exit();
}

// get data bytes
$bytes = array();
foreach(str_split(base64_decode($data->payload)) as $b) {
    $bytes[] = $b;
}


// get temperature
$hex = bin2hex($bytes[0]).bin2hex($bytes[1]).bin2hex($bytes[2]).bin2hex($bytes[3]);
$temperature = unpack('f', hex2bin($hex));

// get humidity
$hex = bin2hex($bytes[4]).bin2hex($bytes[5]).bin2hex($bytes[6]).bin2hex($bytes[7]);
$humidity = unpack('f', hex2bin($hex));

// encode decoded data as json
$decoded_data = json_encode([
    'temperature' => $temperature,
    'humidity' => $humidity,
    'ts' => time()
]);

file_put_contents('data.json', $decoded_data);

echo $decoded_data;

?>