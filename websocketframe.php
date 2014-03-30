<?hh

<<__NativeData("WebSocketFrame")>>
class WebSocketFrame
{
    public function __construct()
    {
    }

    <<__Native>>
    public function getPayload() : mixed;

    <<__Native>>
    public static function parseFromString(string $bytes) : mixed;
}