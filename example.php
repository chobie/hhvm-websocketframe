<?php


$frame = new WebSocketFrame();

$data = file_get_contents("frame2.dat");
$frame = WebSocketFrame::parseFromString($data);

var_dump($data);
var_dump($frame);
var_dump($frame->getPayload());