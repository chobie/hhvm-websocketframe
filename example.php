<?php

//var_dump(ini_get("hhvm.eval.jit"));

$frame = new WebSocketFrame();
//var_dump(WebSocketFrame::OP_BINARY);

$data = file_get_contents("frame2.dat");
$frame = WebSocketFrame::parseFromString($data);

//var_dump($data);
//var_dump($frame);
//var_dump($frame->getPayload());
//echo "================================\n";

$f = new WebSocketFrame();
$f->setPayload("HELO World");
echo $f->serializeToString();

