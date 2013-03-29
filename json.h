#ifndef JSON_H
#define JSON_H

typedef enum {
  JSON_INIT,
  JSON_STRING,
  JSON_VALUE,
  JSON_KEY,
} json_state;

typedef enum {
  JSON_STR,
  JSON_OBJECT,
  JSON_ARRAY,
} json_type;

typedef struct  {
  /* type of this value */
  json_type type;
  /* whether this value is a key or a value */
  json_state kv;
  /* offset of this value in the buffer */
  size_t offset;
  /* length of this value, if it is a string */
  size_t length;
  /* depth of this value in the json file */
  size_t depth;
  /* if type == JSON_ARRAY, index of this value in the array */
  size_t array_index;
  /* the buffer */
  char* buffer;
} json_value;

int parse_json(char* buffer, size_t length, void* user_ptr, void (*callback)(json_value* v, void* usr_ptr));

extern const char* type2str[];

#endif /* JSON_H */
