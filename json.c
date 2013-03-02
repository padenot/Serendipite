#include "macros.h"
#include "json.h"

const char* type2str[] = {
  "str",
  "obj",
  "arr"
};

int parse_json(char* buffer, size_t length, void* user_ptr,
               void (*callback)(json_value* v, void* user_ptr))
{
  size_t offset = 0;
  json_state state = JSON_INIT;
  json_state kvstate = JSON_INIT;
  int depth = 0;
  int string_start = 0;

  while(offset < length) {
    /*LOG(LOG_DEBUG, "%d : \"%c\"", offset, buffer[offset]);*/
    switch(buffer[offset]) {
      case '{': {
        json_value v = {
          JSON_OBJECT,
          kvstate,
          offset,
          0,
          depth,
          0,
          buffer
        };
        kvstate = JSON_KEY;
        callback(&v, user_ptr);
        depth++;
        break;
      };
      case '}':
        depth--;
        break;
      case '"':
        if (state != JSON_STRING) {
          state = JSON_STRING;
          /* string start is the offset of the " */
          string_start = offset;
        } else {
          json_value v = {
            JSON_STR,
            kvstate,
            string_start + 1,
            offset - 1 - string_start,
            depth,
            0,
            buffer
          };
          kvstate = JSON_KEY;
          state = JSON_INIT;
          callback(&v, user_ptr);
        }
        break;
      case ':':
        kvstate = JSON_VALUE;
        break;
      case ',':
      case ' ':
      case '\t':
      case '\n':
      case '\r':
      case '[':
      case ']':
        break;
      default:
        if (depth == 0) {
          LOG(LOG_WARNING, "invalid char %c at offset %zu", buffer[offset], offset);
          return ERROR;
        }
        break;
    }
    offset++;
  }
  return OK;
}

