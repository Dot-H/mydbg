/*
 * Copyright © 2008 Kristian Høgsberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/** \file wayland-util.h
 *
 * \brief Utility classes, functions, and macros.
 */

#ifndef WAYLAND_UTIL_H
#define WAYLAND_UTIL_H

#include <math.h>
#include <stddef.h>
#include <inttypes.h>
#include <stdarg.h>

#ifdef  __cplusplus
extern "C" {
#endif

/** Visibility attribute */
#if defined(__GNUC__) && __GNUC__ >= 4
#define WL_EXPORT __attribute__ ((visibility("default")))
#else
#define WL_EXPORT
#endif

/** Deprecated attribute */
#if defined(__GNUC__) && __GNUC__ >= 4
#define WL_DEPRECATED __attribute__ ((deprecated))
#else
#define WL_DEPRECATED
#endif

/**
 * Printf-style argument attribute
 *
 * \param x Ordinality of the format string argument
 * \param y Ordinality of the argument to check against the format string
 *
 * \sa https://gcc.gnu.org/onlinedocs/gcc-3.2.1/gcc/Function-Attributes.html
 */
#if defined(__GNUC__) && __GNUC__ >= 4
#define WL_PRINTF(x, y) __attribute__((__format__(__printf__, x, y)))
#else
#define WL_PRINTF(x, y)
#endif

/**
 * Protocol message signature
 *
 * A wl_message describes the signature of an actual protocol message, such as a
 * request or event, that adheres to the Wayland protocol wire format. The
 * protocol implementation uses a wl_message within its demarshal machinery for
 * decoding messages between a compositor and its clients. In a sense, a
 * wl_message is to a protocol message like a class is to an object.
 *
 * The `name` of a wl_message is the name of the corresponding protocol message.
 * The `signature` is an ordered list of symbols representing the data types
 * of message arguments and, optionally, a protocol version and indicators for
 * nullability. A leading integer in the `signature` indicates the _since_
 * version of the protocol message. A `?` preceding a data type symbol indicates
 * that the following argument type is nullable. When no arguments accompany a
 * message, `signature` is an empty string.
 *
 * * `i`: int
 * * `u`: uint
 * * `f`: fixed
 * * `s`: string
 * * `o`: object
 * * `n`: new_id
 * * `a`: array
 * * `h`: fd
 * * `?`: following argument is nullable
 *
 * While demarshaling primitive arguments is straightforward, when demarshaling
 * messages containing `object` or `new_id` arguments, the protocol
 * implementation often must determine the type of the object. The `types` of a
 * wl_message is an array of wl_interface references that correspond to `o` and
 * `n` arguments in `signature`, with `NULL` placeholders for arguments with
 * non-object types.
 *
 * Consider the protocol event wl_display `delete_id` that has a single `uint`
 * argument. The wl_message is:
 *
 * \code
 * { "delete_id", "u", [NULL] }
 * \endcode
 *
 * Here, the message `name` is `"delete_id"`, the `signature` is `"u"`, and the
 * argument `types` is `[NULL]`, indicating that the `uint` argument has no
 * corresponding wl_interface since it is a primitive argument.
 *
 * In contrast, consider a `wl_foo` interface supporting protocol request `bar`
 * that has existed since version 2, and has two arguments: a `uint` and an
 * object of type `wl_baz_interface` that may be `NULL`. Such a `wl_message`
 * might be:
 *
 * \code
 * { "bar", "2u?o", [NULL, &wl_baz_interface] }
 * \endcode
 *
 * Here, the message `name` is `"bar"`, and the `signature` is `"2u?o"`. Notice
 * how the `2` indicates the protocol version, the `u` indicates the first
 * argument type is `uint`, and the `?o` indicates that the second argument
 * is an object that may be `NULL`. Lastly, the argument `types` array indicates
 * that no wl_interface corresponds to the first argument, while the type
 * `wl_baz_interface` corresponds to the second argument.
 *
 * \sa wl_argument
 * \sa wl_interface
 * \sa <a href="https://wayland.freedesktop.org/docs/html/ch04.html#sect-Protocol-Wire-Format">Wire Format</a>
 */
struct wl_message {
	/** Message name */
	const char *name;
	/** Message signature */
	const char *signature;
	/** Object argument interfaces */
	const struct wl_interface **types;
};

/**
 * Protocol object interface
 *
 * A wl_interface describes the API of a protocol object defined in the Wayland
 * protocol specification. The protocol implementation uses a wl_interface
 * within its marshalling machinery for encoding client requests.
 *
 * The `name` of a wl_interface is the name of the corresponding protocol
 * interface, and `version` represents the version of the interface. The members
 * `method_count` and `event_count` represent the number of `methods` (requests)
 * and `events` in the respective wl_message members.
 *
 * For example, consider a protocol interface `foo`, marked as version `1`, with
 * two requests and one event.
 *
 * \code
 * <interface name="foo" version="1">
 *   <request name="a"></request>
 *   <request name="b"></request>
 *   <event name="c"></event>
 * </interface>
 * \endcode
 *
 * Given two wl_message arrays `foo_requests` and `foo_events`, a wl_interface
 * for `foo` might be:
 *
 * \code
 * struct wl_interface foo_interface = {
 *         "foo", 1,
 *         2, foo_requests,
 *         1, foo_events
 * };
 * \endcode
 *
 * \note The server side of the protocol may define interface <em>implementation
 *       types</em> that incorporate the term `interface` in their name. Take
 *       care to not confuse these server-side `struct`s with a wl_interface
 *       variable whose name also ends in `interface`. For example, while the
 *       server may define a type `struct wl_foo_interface`, the client may
 *       define a `struct wl_interface wl_foo_interface`.
 *
 * \sa wl_message
 * \sa wl_proxy
 * \sa <a href="https://wayland.freedesktop.org/docs/html/ch04.html#sect-Protocol-Interfaces">Interfaces</a>
 * \sa <a href="https://wayland.freedesktop.org/docs/html/ch04.html#sect-Protocol-Versioning">Versioning</a>
 */
struct wl_interface {
	/** Interface name */
	const char *name;
	/** Interface version */
	int version;
	/** Number of methods (requests) */
	int method_count;
	/** Method (request) signatures */
	const struct wl_message *methods;
	/** Number of events */
	int event_count;
	/** Event signatures */
	const struct wl_message *events;
};

/** \class wl_list
 *
 * \brief Doubly-linked list
 *
 * On its own, an instance of `struct wl_list` represents the sentinel head of
 * a doubly-linked list, and must be initialized using wl_list_init().
 * When empty, the list head's `next` and `prev` members point to the list head
 * itself, otherwise `next` references the first element in the list, and `prev`
 * refers to the last element in the list.
 *
 * Use the `struct wl_list` type to represent both the list head and the links
 * between elements within the list. Use wl_list_empty() to determine if the
 * list is empty in O(1).
 *
 * All elements in the list must be of the same type. The element type must have
 * a `struct wl_list` member, often named `link` by convention. Prior to
 * insertion, there is no need to initialize an element's `link` - invoking
 * wl_list_init() on an individual list element's `struct wl_list` member is
 * unnecessary if the very next operation is wl_list_insert(). However, a
 * common idiom is to initialize an element's `link` prior to removal - ensure
 * safety by invoking wl_list_init() before wl_list_remove().
 *
 * Consider a list reference `struct wl_list foo_list`, an element type as
 * `struct element`, and an element's link member as `struct wl_list link`.
 *
 * The following code initializes a list and adds three elements to it.
 *
 * \code
 * struct wl_list foo_list;
 *
 * struct element {
 *         int foo;
 *         struct wl_list link;
 * };
 * struct element e1, e2, e3;
 *
 * wl_list_init(&foo_list);
 * wl_list_insert(&foo_list, &e1.link);   // e1 is the first element
 * wl_list_insert(&foo_list, &e2.link);   // e2 is now the first element
 * wl_list_insert(&e2.link, &e3.link); // insert e3 after e2
 * \endcode
 *
 * The list now looks like <em>[e2, e3, e1]</em>.
 *
 * The `wl_list` API provides some iterator macros. For example, to iterate
 * a list in ascending order:
 *
 * \code
 * struct element *e;
 * wl_list_for_each(e, foo_list, link) {
 *         do_something_with_element(e);
 * }
 * \endcode
 *
 * See the documentation of each iterator for details.
 * \sa http://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/tree/include/linux/list.h
 */
struct wl_list {
	/** Previous list element */
	struct wl_list *prev;
	/** Next list element */
	struct wl_list *next;
};

/**
 * Initializes the list.
 *
 * \param list List to initialize
 *
 * \memberof wl_list
 */
void
wl_list_init(struct wl_list *list);

/**
 * Inserts an element into the list, after the element represented by \p list.
 * When \p list is a reference to the list itself (the head), set the containing
 * struct of \p elm as the first element in the list.
 *
 * \note If \p elm is already part of a list, inserting it again will lead to
 *       list corruption.
 *
 * \param list List element after which the new element is inserted
 * \param elm Link of the containing struct to insert into the list
 *
 * \memberof wl_list
 */
void
wl_list_insert(struct wl_list *list, struct wl_list *elm);

/**
 * Removes an element from the list.
 *
 * \note This operation leaves \p elm in an invalid state.
 *
 * \param elm Link of the containing struct to remove from the list
 *
 * \memberof wl_list
 */
void
wl_list_remove(struct wl_list *elm);

/**
 * Determines the length of the list.
 *
 * \note This is an O(n) operation.
 *
 * \param list List whose length is to be determined
 *
 * \return Number of elements in the list
 *
 * \memberof wl_list
 */
int
wl_list_length(const struct wl_list *list);

/**
 * Determines if the list is empty.
 *
 * \param list List whose emptiness is to be determined
 *
 * \return 1 if empty, or 0 if not empty
 *
 * \memberof wl_list
 */
int
wl_list_empty(const struct wl_list *list);

/**
 * Inserts all of the elements of one list into another, after the element
 * represented by \p list.
 *
 * \note This leaves \p other in an invalid state.
 *
 * \param list List element after which the other list elements will be inserted
 * \param other List of elements to insert
 *
 * \memberof wl_list
 */
void
wl_list_insert_list(struct wl_list *list, struct wl_list *other);

/**
 * Retrieves a pointer to a containing struct, given a member name.
 *
 * This macro allows "conversion" from a pointer to a member to its containing
 * struct. This is useful if you have a contained item like a wl_list,
 * wl_listener, or wl_signal, provided via a callback or other means, and would
 * like to retrieve the struct that contains it.
 *
 * To demonstrate, the following example retrieves a pointer to
 * `example_container` given only its `destroy_listener` member:
 *
 * \code
 * struct example_container {
 *         struct wl_listener destroy_listener;
 *         // other members...
 * };
 *
 * void example_container_destroy(struct wl_listener *listener, void *data)
 * {
 *         struct example_container *ctr;
 *
 *         ctr = wl_container_of(listener, ctr, destroy_listener);
 *         // destroy ctr...
 * }
 * \endcode
 *
 * \note `sample` need not be a valid pointer. A null or uninitialised pointer
 *       is sufficient.
 *
 * \param ptr Valid pointer to the contained member
 * \param sample Pointer to a struct whose type contains \p ptr
 * \param member Named location of \p ptr within the \p sample type
 *
 * \return The container for the specified pointer
 */
#define wl_container_of(ptr, sample, member)				\
	(__typeof__(sample))((char *)(ptr) -				\
			     offsetof(__typeof__(*sample), member))

/**
 * Iterates over a list.
 *
 * This macro expresses a for-each iterator for wl_list. Given a list and
 * wl_list link member name (often named `link` by convention), this macro
 * assigns each element in the list to \p pos, which can then be referenced in
 * a trailing code block. For example, given a wl_list of `struct message`
 * elements:
 *
 * \code
 * struct message {
 *         char *contents;
 *         wl_list link;
 * };
 *
 * struct wl_list *message_list;
 * // Assume message_list now "contains" many messages
 *
 * struct message *m;
 * wl_list_for_each(m, message_list, link) {
 *         do_something_with_message(m);
 * }
 * \endcode
 *
 * \param pos Cursor that each list element will be assigned to
 * \param head Head of the list to iterate over
 * \param member Name of the link member within the element struct
 *
 * \relates wl_list
 */
#define wl_list_for_each(pos, head, member)				\
	for (pos = wl_container_of((head)->next, pos, member);	\
	     &pos->member != (head);					\
	     pos = wl_container_of(pos->member.next, pos, member))

/**
 * Iterates over a list, safe against removal of the list element.
 *
 * \note Only removal of the current element, \p pos, is safe. Removing
 *       any other element during traversal may lead to a loop malfunction.
 *
 * \sa wl_list_for_each()
 *
 * \param pos Cursor that each list element will be assigned to
 * \param tmp Temporary pointer of the same type as \p pos
 * \param head Head of the list to iterate over
 * \param member Name of the link member within the element struct
 *
 * \relates wl_list
 */
#define wl_list_for_each_safe(pos, tmp, head, member)			\
	for (pos = wl_container_of((head)->next, pos, member),		\
	     tmp = wl_container_of((pos)->member.next, tmp, member);	\
	     &pos->member != (head);					\
	     pos = tmp,							\
	     tmp = wl_container_of(pos->member.next, tmp, member))

/**
 * Iterates backwards over a list.
 *
 * \sa wl_list_for_each()
 *
 * \param pos Cursor that each list element will be assigned to
 * \param head Head of the list to iterate over
 * \param member Name of the link member within the element struct
 *
 * \relates wl_list
 */
#define wl_list_for_each_reverse(pos, head, member)			\
	for (pos = wl_container_of((head)->prev, pos, member);	\
	     &pos->member != (head);					\
	     pos = wl_container_of(pos->member.prev, pos, member))

/**
 * Iterates backwards over a list, safe against removal of the list element.
 *
 * \note Only removal of the current element, \p pos, is safe. Removing
 *       any other element during traversal may lead to a loop malfunction.
 *
 * \sa wl_list_for_each()
 *
 * \param pos Cursor that each list element will be assigned to
 * \param tmp Temporary pointer of the same type as \p pos
 * \param head Head of the list to iterate over
 * \param member Name of the link member within the element struct
 *
 * \relates wl_list
 */
#define wl_list_for_each_reverse_safe(pos, tmp, head, member)		\
	for (pos = wl_container_of((head)->prev, pos, member),	\
	     tmp = wl_container_of((pos)->member.prev, tmp, member);	\
	     &pos->member != (head);					\
	     pos = tmp,							\
	     tmp = wl_container_of(pos->member.prev, tmp, member))

/**
 * Fixed-point number
 *
 * A `wl_fixed_t` is a 24.8 signed fixed-point number with a sign bit, 23 bits
 * of integer precision and 8 bits of decimal precision. Consider `wl_fixed_t`
 * as an opaque struct with methods that facilitate conversion to and from
 * `double` and `int` types.
 */
typedef int32_t wl_fixed_t;

/**
 * Converts a fixed-point number to a floating-point number.
 *
 * \param f Fixed-point number to convert
 *
 * \return Floating-point representation of the fixed-point argument
 */
static inline double
wl_fixed_to_double(wl_fixed_t f)
{
	union {
		double d;
		int64_t i;
	} u;

	u.i = ((1023LL + 44LL) << 52) + (1LL << 51) + f;

	return u.d - (3LL << 43);
}

/**
 * Converts a floating-point number to a fixed-point number.
 *
 * \param d Floating-point number to convert
 *
 * \return Fixed-point representation of the floating-point argument
 */
static inline wl_fixed_t
wl_fixed_from_double(double d)
{
	union {
		double d;
		int64_t i;
	} u;

	u.d = d + (3LL << (51 - 8));

	return u.i;
}

/**
 * Converts a fixed-point number to an integer.
 *
 * \param f Fixed-point number to convert
 *
 * \return Integer component of the fixed-point argument
 */
static inline int
wl_fixed_to_int(wl_fixed_t f)
{
	return f / 256;
}

/**
 * Converts an integer to a fixed-point number.
 *
 * \param i Integer to convert
 *
 * \return Fixed-point representation of the integer argument
 */
static inline wl_fixed_t
wl_fixed_from_int(int i)
{
	return i * 256;
}

/**
 * Protocol message argument data types
 *
 * This union represents all of the argument types in the Wayland protocol wire
 * format. The protocol implementation uses wl_argument within its marshalling
 * machinery for dispatching messages between a client and a compositor.
 *
 * \sa wl_message
 * \sa wl_interface
 * \sa <a href="https://wayland.freedesktop.org/docs/html/ch04.html#sect-Protocol-wire-Format">Wire Format</a>
 */
union wl_argument {
	int32_t i;           /**< `int`    */
	uint32_t u;          /**< `uint`   */
	wl_fixed_t f;        /**< `fixed`  */
	const char *s;       /**< `string` */
	struct wl_object *o; /**< `object` */
	uint32_t n;          /**< `new_id` */
	struct wl_array *a;  /**< `array`  */
	int32_t h;           /**< `fd`     */
};

/**
 * Dispatcher function type alias
 *
 * A dispatcher is a function that handles the emitting of callbacks in client
 * code. For programs directly using the C library, this is done by using
 * libffi to call function pointers. When binding to languages other than C,
 * dispatchers provide a way to abstract the function calling process to be
 * friendlier to other function calling systems.
 *
 * A dispatcher takes five arguments: The first is the dispatcher-specific
 * implementation associated with the target object. The second is the object
 * upon which the callback is being invoked (either wl_proxy or wl_resource).
 * The third and fourth arguments are the opcode and the wl_message
 * corresponding to the callback. The final argument is an array of arguments
 * received from the other process via the wire protocol.
 *
 * \param "const void *" Dispatcher-specific implementation data
 * \param "void *" Callback invocation target (wl_proxy or `wl_resource`)
 * \param uint32_t Callback opcode
 * \param "const struct wl_message *" Callback message signature
 * \param "union wl_argument *" Array of received arguments
 *
 * \return 0 on success, or -1 on failure
 */
typedef int (*wl_dispatcher_func_t)(const void *, void *, uint32_t,
				    const struct wl_message *,
				    union wl_argument *);

/**
 * Log function type alias
 *
 * The C implementation of the Wayland protocol abstracts the details of
 * logging. Users may customize the logging behavior, with a function conforming
 * to the `wl_log_func_t` type, via `wl_log_set_handler_client` and
 * `wl_log_set_handler_server`.
 *
 * A `wl_log_func_t` must conform to the expectations of `vprintf`, and
 * expects two arguments: a string to write and a corresponding variable
 * argument list. While the string to write may contain format specifiers and
 * use values in the variable argument list, the behavior of any `wl_log_func_t`
 * depends on the implementation.
 *
 * \note Take care to not confuse this with `wl_protocol_logger_func_t`, which
 *       is a specific server-side logger for requests and events.
 *
 * \param "const char *" String to write to the log, containing optional format
 *                       specifiers
 * \param "va_list" Variable argument list
 *
 * \sa wl_log_set_handler_client
 * \sa wl_log_set_handler_server
 */
typedef void (*wl_log_func_t)(const char *, va_list) WL_PRINTF(1, 0);

/**
 * Return value of an iterator function
 *
 * \sa wl_client_for_each_resource_iterator_func_t
 * \sa wl_client_for_each_resource
 */
enum wl_iterator_result {
	/** Stop the iteration */
	WL_ITERATOR_STOP,
	/** Continue the iteration */
	WL_ITERATOR_CONTINUE
};

#ifdef  __cplusplus
}
#endif

#endif