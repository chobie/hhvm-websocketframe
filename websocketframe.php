<?hh

<<__NativeData("WebSocketFrame")>>
class WebSocketFrame
{
    public function __construct()
    {
    }

    <<__Native>>
    public function getPayload() : string;

    <<__Native>>
    public function getOpcode() : int;

    <<__Native>>
    public static function parseFromString(string $bytes) : mixed;
}