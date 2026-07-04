#ifndef XCB_XCB_H
#define XCB_XCB_H
#include <stdint.h>
#include <stdlib.h>
typedef struct xcb_connection_t xcb_connection_t;
typedef int xcb_atom_t;
/* Cookie types used in dwm */
typedef struct { unsigned int sequence; } xcb_intern_atom_cookie_t;
typedef struct { unsigned int sequence; } xcb_get_property_cookie_t;
typedef struct { unsigned int sequence; } xcb_get_window_attributes_cookie_t;
typedef struct { unsigned int sequence; } xcb_get_geometry_cookie_t;
typedef struct { unsigned int sequence; } xcb_query_tree_cookie_t;
/* Generic event */
typedef struct { uint8_t response_type; uint8_t pad0; uint16_t sequence; uint32_t len; } xcb_generic_event_t;
typedef struct { uint8_t response_type; uint8_t pad0; uint16_t sequence; uint32_t length; } xcb_generic_error_t;
/* from xproto.h */
typedef uint32_t xcb_window_t;
enum { XCB_ATOM_NONE = 0 };

/* from xcb.h */
xcb_connection_t *xcb_connect(const char *displayname, int *screenp);
void xcb_disconnect(xcb_connection_t *c);
xcb_intern_atom_cookie_t xcb_intern_atom(xcb_connection_t *c, uint8_t only_if_exists, uint16_t name_len, const char *name);
#endif
