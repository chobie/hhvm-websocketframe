#include "hphp/runtime/base/base-includes.h"
#include "hphp/runtime/vm/native-data.h"

#ifdef _MSC_VER
  #if defined(_M_IX86) && \
      !defined(PHP_WEBSOCKETFRAME_DISABLE_LITTLE_ENDIAN_OPT_FOR_TEST)
    #define WEBSOCKETFRAME_LITTLE_ENDIAN 1
  #endif
  #if _MSC_VER >= 1300
    // If MSVC has "/RTCc" set, it will complain about truncating casts at
    // runtime.  This file contains some intentional truncating casts.
    #pragma runtime_checks("c", off)
  #endif
#else
  #include <sys/param.h>   // __BYTE_ORDER
  #ifdef __APPLE__
    #define __BIG_ENDIAN __DARWIN_BIG_ENDIAN
    #define __LITTLE_ENDIAN __DARWIN_LITTLE_ENDIAN
    #define __BYTE_ORDER __DARWIN_BYTE_ORDER
  #endif

  #if defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN && \
      !defined(PHP_WEBSOCKETFRAME_DISABLE_LITTLE_ENDIAN_OPT_FOR_TEST)
    #define WEBSOCKETFRAME_LITTLE_ENDIAN 1
  #endif
#endif

namespace HPHP {

//extern const StaticString s_WebSocketFrame;

class WebSocketFrame{
 public:
  WebSocketFrame() :
    m_mutable(true), m_fin(true),
    m_rsv1(false), m_rsv2(false), m_rsv3(false), m_mask(false){

    memset(m_mask_key, 0, 4);
    m_opcode = 1;
    m_payload = NULL;
    m_payload_length = 0;

  };

  ~WebSocketFrame();

  WebSocketFrame(const WebSocketFrame&) = delete;
  WebSocketFrame& operator=(const WebSocketFrame& src) {
    return *this;
  }

  void ParseFromString(const String &data);
  String SerializeToString();

  void SetPayload(const String& payload) {
    if (m_mutable) {
      if (m_payload_length > 0) {
        free(m_payload);
      }

      m_payload = static_cast<unsigned char*>(malloc(sizeof(payload.length())));
      memcpy(static_cast<void*>(m_payload), payload.c_str(), payload.length());
      m_payload_length = payload.length();
    }
  }

  String GetPayload() {
    return String(reinterpret_cast<char *>(m_payload), CopyString);
  }

  int64_t GetOpcode() {
    return static_cast<int64_t>(m_opcode);
  }

  static Class* c_WebSocketFrame;
  bool m_mutable;
  bool m_fin;
  bool m_rsv1;
  bool m_rsv2;
  bool m_rsv3;
  bool m_mask;
  unsigned char m_mask_key[4];
  uint16_t m_opcode;
  unsigned char *m_payload;
  uint64_t m_payload_length;
};

WebSocketFrame::~WebSocketFrame() {
  if (m_payload_length > 0) {
    free(m_payload);
  }
}

String WebSocketFrame::SerializeToString() {
  unsigned char *buffer;
  size_t frame_size = 2, offset = 2;

  if (m_payload_length == 0) {
    //zend_throw_exception_ex(spl_ce_LogicException, 0 TSRMLS_CC, "empty payload");
    return String("");
  }

  if (m_mask) {
    frame_size += 4;
  }

  if (m_payload_length >= 0x7e && m_payload_length <= 0x10000) {
    frame_size += 2;
  } else if (m_payload_length >= 0x10000) {
    frame_size += 8;
  }

  frame_size += m_payload_length;
  buffer = static_cast<unsigned char*>(malloc(sizeof(unsigned char) * frame_size + 1));
  memset(buffer, 0, frame_size + 1);

  buffer[0] = 0;
  if (m_fin) {
    buffer[0] |= 0x80;
  }
  if (m_rsv1) {
    buffer[0] |= 0x10;
  }
  if (m_rsv2) {
    buffer[0] |= 0x20;
  }
  if (m_rsv3) {
    buffer[0] |= 0x40;
  }
  // type
  buffer[0] |= m_opcode;
  buffer[1] = 0;

  if (m_payload_length < 0x7e) {
    buffer[1] |= m_payload_length;
  } else if (m_payload_length < 0x10000) {
    union {
      uint16_t length;
      uint8_t buffer[2];
    } u;

#ifdef WEBSOCKETFRAME_LITTLE_ENDIAN
    u.length = m_payload_length;
    u.length = (u.length >> 8) | (u.length << 8);
#else
    u.length = m_payload_length;
#endif

    buffer[1] |= 0x7e;

    memcpy(&buffer[offset], u.buffer, 2);
    offset += 2;
  } else {
    union {
      uint64_t length;
      uint8_t buffer[8];
    } u;

#ifdef WEBSOCKETFRAME_LITTLE_ENDIAN
    u.length = m_payload_length;
    u.length = (u.length >> 56) |
        ((u.length<<40) & 0x00FF000000000000) |
        ((u.length<<24) & 0x0000FF0000000000) |
        ((u.length<<8) & 0x000000FF00000000) |
        ((u.length>>8) & 0x00000000FF000000) |
        ((u.length>>24) & 0x0000000000FF0000) |
        ((u.length>>40) & 0x000000000000FF00) |
        (u.length << 56);
#else
    u.length = m_payload_length;
#endif

    buffer[1] |= 0x7f;
    memcpy(&buffer[offset], &u.length, 8);
    offset += 8;
  }

  if (m_mask) {
    // TODO: implement MASK DATA
  }

  memcpy(&buffer[offset], m_payload, m_payload_length);

  String result = String(const_cast<const char*>(reinterpret_cast<char*>(buffer)), frame_size, CopyString);
  free(buffer);
  return result;
}

void WebSocketFrame::ParseFromString(const String &data) {
    const char *bytes = data.c_str();
    int32_t offset = 2;

    m_fin = ((bytes[0] & 0x80) >> 7);
    m_opcode = (bytes[0] & 0x0f);
    m_mask = ((bytes[1] & 0x80) >> 7);
    m_payload_length = static_cast<uint64_t>((bytes[1] & 0x7f));

    if (m_payload_length == static_cast<uint64_t>(0x7e)) {
      uint16_t length;

#ifdef WEBSOCKETFRAME_LITTLE_ENDIAN
      memcpy(&length, &bytes[offset], 2);
      m_payload_length = (length >> 8) | (length << 8);
#else
      memcpy(&length, &bytes[offset], 2);
      m_payload_length = (uint64_t)length;
#endif

      offset += 2;
    } else if (m_payload_length == 0x7f) {
      uint64_t length;

#ifdef WEBSOCKETFRAME_LITTLE_ENDIAN
      memcpy(&length, &bytes[offset], 8);
      m_payload_length = (uint64_t)length;
#else
      memcpy(&length, &bytes[offset], 8);
      m_payload_length = (length >> 56) |
                      ((length<<40) & 0x00FF000000000000) |
                      ((length<<24) & 0x0000FF0000000000) |
                      ((length<<8) & 0x000000FF00000000) |
                      ((length>>8) & 0x00000000FF000000) |
                      ((length>>24) & 0x0000000000FF0000) |
                      ((length>>40) & 0x000000000000FF00) |
                      (length << 56);
#endif

      memcpy(reinterpret_cast<void*>(length), reinterpret_cast<void*>(bytes[offset]), 8);
      offset += 8;
    }

    if (m_mask) {
      memcpy(reinterpret_cast<void*>(m_mask_key), &bytes[offset], 4);
      offset += 4;
    }
    m_payload = static_cast<unsigned char*>(calloc(1, sizeof(unsigned char) * m_payload_length));
    memcpy(m_payload, &bytes[offset], m_payload_length);

    if (m_mask) {
      uint64_t i;

      for (i = 0; i < m_payload_length; i++) {
        m_payload[i] = m_payload[i] ^ m_mask_key[i % 4];
      }
    }
    m_mutable = 0;
}

const StaticString s_WebSocketFrame("WebSocketFrame");
Class* WebSocketFrame::c_WebSocketFrame = nullptr;

////

static void HHVM_METHOD(websocketframe, setPayload, const String& payload) {
  WebSocketFrame* frame = Native::data<WebSocketFrame>(this_.get());

  frame->SetPayload(payload);
}


static Variant HHVM_METHOD(websocketframe, getPayload) {
  WebSocketFrame* frame = Native::data<WebSocketFrame>(this_.get());

  return frame->GetPayload();
}

static int64_t HHVM_METHOD(websocketframe, getOpcode) {
  WebSocketFrame* frame = Native::data<WebSocketFrame>(this_.get());

  return frame->GetOpcode();
}

static String HHVM_METHOD(websocketframe, serializeToString) {
   WebSocketFrame* frame = Native::data<WebSocketFrame>(this_.get());

   return frame->SerializeToString();
}

static Variant HHVM_STATIC_METHOD(websocketframe, parseFromString, const String& bytes) {
  if (!WebSocketFrame::c_WebSocketFrame) {
    WebSocketFrame::c_WebSocketFrame = Unit::lookupClass(s_WebSocketFrame.get());
    assert(WebSocketFrame::c_WebSocketFrame);
  }
  Object object = ObjectData::newInstance(WebSocketFrame::c_WebSocketFrame);

  WebSocketFrame* frame = Native::data<WebSocketFrame>(object.get());
  frame->ParseFromString(bytes);
  //WebSocketFrame* frame = object.getTyped<WebSocketFrame>();
  return object;
}

namespace {

enum OpcodeVariant {
  OP_CONTENUATION = 0x00,
  OP_TEXT = 0x01,
  OP_BINARY = 0x02,
  OP_PING = 0x08,
  OP_PONG = 0x09
};

const StaticString
  s_WebSocketFrame_OP_CONTENUATION("OP_CONTENUATION"),
  s_WebSocketFrame_OP_TEXT("OP_TEXT"),
  s_WebSocketFrame_OP_BINARY("OP_BINARY"),
  s_WebSocketFrame_OP_PING("OP_PING"),
  s_WebSocketFrame_OP_PONG("OP_PONG");

static class WebSocketFrameExtension : public Extension {

public:
  WebSocketFrameExtension() : Extension("websocketframe") {}

  virtual void moduleInit() {
    HHVM_ME(websocketframe, setPayload);
    HHVM_ME(websocketframe, getPayload);
    HHVM_ME(websocketframe, getOpcode);
    HHVM_ME(websocketframe, serializeToString);
    HHVM_STATIC_ME(websocketframe, parseFromString);

    Native::registerClassConstant<KindOfInt64>(s_WebSocketFrame.get(), s_WebSocketFrame_OP_CONTENUATION.get(), OP_CONTENUATION);
    Native::registerClassConstant<KindOfInt64>(s_WebSocketFrame.get(), s_WebSocketFrame_OP_TEXT.get(), OP_TEXT);
    Native::registerClassConstant<KindOfInt64>(s_WebSocketFrame.get(), s_WebSocketFrame_OP_BINARY.get(), OP_BINARY);
    Native::registerClassConstant<KindOfInt64>(s_WebSocketFrame.get(), s_WebSocketFrame_OP_PING.get(), OP_PING);
    Native::registerClassConstant<KindOfInt64>(s_WebSocketFrame.get(), s_WebSocketFrame_OP_PONG.get(), OP_PONG);

    Native::registerNativeDataInfo<WebSocketFrame>(s_WebSocketFrame.get());
    loadSystemlib();
  }
} s_websocketframe_extension;
}

HHVM_GET_MODULE(websocketframe)
} // namespace HPHP