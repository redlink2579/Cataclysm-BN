#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <utility>

#include "demangle.h"

class translation;

template<typename T>
class string_id;

namespace cata
{

class string_formatter;

// wrapper to allow calling string_formatter::throw_error before the definition of string_formatter
[[noreturn]]
void throw_error( const string_formatter &, const std::string & );
// wrapper to access string_formatter::temp_buffer before the definition of string_formatter
std::string_view string_formatter_set_temp_buffer( const string_formatter &, std::string_view );
// Handle currently active exception from string_formatter and return it as string
std::string handle_string_format_error();

/**
 * @defgroup string_formatter_convert Convert functions for @ref string_formatter
 *
 * The `convert` functions here are used to convert the input value of
 * @ref string_formatter::parse into the requested type, as defined by the format specifiers.
 *
 * @tparam T the input type, as given by the call to `string_format`.
 * @tparam RT the requested type. The `convert` functions return such a value or they throw
 * an exception via @ref throw_error.
 *
 * Each function has the same parameters:
 * First parameter defined the requested type. The value of the pointer is ignored, callers
 * should use a (properly casted) `nullptr`. It is required to "simulate" overloading the
 * return value. E.g. `long convert(long*, int)` and `short convert(short*, int)` both convert
 * a input value of type `int`, but the first converts to `long` and the second converts to
 * `short`. Without the first parameters their signature would be identical.
 * The second parameter is used to call @ref throw_error / @ref string_formatter_set_temp_buffer.
 * The third parameter is the input value that is to be converted.
 * The fourth parameter is a dummy value, it is always ignored, callers should use `0` here.
 * It is used so the fallback with the variadic arguments is *only* chosen when no other
 * overload matches.
 */
/**@{*/
// Test for arithmetic type, *excluding* bool. printf can not handle bool, so can't we.
template<typename T>
using is_numeric = std::conditional_t <
                   std::is_arithmetic_v<std::decay_t<T>> &&
                   !std::is_same_v<std::decay_t<T>, bool>, std::true_type, std::false_type >;
// Test for integer type (not floating point, not bool).
template<typename T>
using is_integer = std::conditional_t < is_numeric<T>::value &&
                   !std::is_floating_point_v<std::decay_t<T>>, std::true_type,
                   std::false_type >;
template<typename T>
using is_char =
    std::conditional_t<std::is_same_v<std::decay_t<T>, char>, std::true_type, std::false_type>;
// Test for std::string type.
template<typename T>
using is_string =
    std::conditional_t<std::is_same_v<std::decay_t<T>, std::string>, std::true_type, std::false_type>;
// Test for std::string_view type.
template<typename T>
using is_string_view =
    std::conditional_t<std::is_same_v<std::decay_t<T>, std::string_view>, std::true_type, std::false_type>;
// Test for c-string type.
template<typename T>
using is_cstring = std::conditional_t <
                   std::is_same_v<std::decay_t<T>, const char *> ||
                   std::is_same_v<std::decay_t<T>, char *>, std::true_type, std::false_type >;
// Test for class translation
template<typename T>
using is_translation = std::conditional_t <
                       std::is_same_v<std::decay_t<T>, translation>, std::true_type,
                       std::false_type >;
// Test for string_id<T>
template <typename, template <typename> class>
struct is_instance_of : std::false_type {};
template <typename T, template <typename> class TMPL>
struct is_instance_of<TMPL<T>, TMPL> : std::true_type {};
template<typename T>
using is_string_id = std::conditional_t <
                     is_instance_of<std::decay_t<T>, string_id>::value, std::true_type,
                     std::false_type >;

template<typename RT, typename T>
inline typename std::enable_if < is_integer<RT>::value &&is_integer<T>::value,
       RT >::type convert( RT *, const string_formatter &, T &&value, int )
{
    return value;
}
template<typename RT, typename T>
inline typename std::enable_if < is_integer<RT>::value
&&std::is_enum<typename std::decay<T>::type>::value,
RT >::type convert( RT *, const string_formatter &, T &&value, int )
{
    return static_cast<RT>( value );
}
template<typename RT, typename T>
inline typename std::enable_if < std::is_floating_point<RT>::value &&is_numeric<T>::value
&&!is_integer<T>::value, RT >::type convert( RT *, const string_formatter &, T &&value, int )
{
    return value;
}
template<typename RT, typename T>
inline typename std::enable_if < std::is_same<RT, void *>::value
&&std::is_pointer<typename std::decay<T>::type>::value, void * >::type convert( RT *,
        const string_formatter &, T &&value, int )
{
    return const_cast<std::remove_const_t<std::remove_pointer_t<std::decay_t<T>>> *>
           ( value );
}
template<typename RT, typename T>
inline typename std::enable_if < std::is_same<RT, std::string_view>::value &&is_string<T>::value,
       std::string_view >::type convert( RT *, const string_formatter &, T &&value, int )
{
    return value;
}
template<typename RT, typename T>
inline std::string_view
convert( RT *, const string_formatter &, T &&value, int )
requires( std::is_same_v<RT, std::string_view >
          &&is_string_view<T>::value )
{
    return value;
}
template<typename RT, typename T>
inline typename std::enable_if < std::is_same<RT, std::string_view>::value &&is_cstring<T>::value,
       std::string_view >::type convert( RT *, const string_formatter &, T &&value, int )
{
    return value;
}
template<typename RT, typename T>
inline typename std::enable_if < std::is_same<RT, std::string_view>::value
&&is_translation<T>::value,
std::string_view >::type convert( RT *, const string_formatter &sf, T &&value, int )
{
    return string_formatter_set_temp_buffer( sf, value.translated() );
}
template<typename RT, typename T>
inline typename std::enable_if < std::is_same<RT, std::string_view>::value &&is_string_id<T>::value,
       std::string_view >::type convert( RT *, const string_formatter &sf, T &&value, int )
{
    return string_formatter_set_temp_buffer( sf, value.str() );
}
template<typename RT, typename T>
inline typename std::enable_if < std::is_same<RT, std::string_view>::value &&is_numeric<T>::value
&&!is_char<T>::value, std::string_view >::type convert( RT *, const string_formatter &sf, T &&value,
        int )
{
    return string_formatter_set_temp_buffer( sf, std::to_string( value ) );
}
template<typename RT, typename T>
inline typename std::enable_if < std::is_same<RT, std::string_view>::value &&is_numeric<T>::value
&&is_char<T>::value, std::string_view >::type convert( RT *, const string_formatter &sf, T &&value,
        int )
{
    return string_formatter_set_temp_buffer( sf, std::string( 1, value ) );
}
// Catch all remaining conversions (the '...' makes this the lowest overload priority).
// The static_assert is used to restrict the input type to those that can actually be printed,
// calling `string_format` with an unknown type will trigger a compile error because no other
// `convert` function will match, while this one will give a static_assert error.
template<typename RT, typename T>
// NOLINTNEXTLINE(cert-dcl50-cpp)
inline RT convert( RT *, const string_formatter &sf, T &&, ... )
{
    static_assert( std::is_pointer_v<std::decay_t<T>> ||
                   is_numeric<T>::value ||
                   is_string<T>::value ||
                   is_string_view<T>::value ||
                   is_char<T>::value ||
                   std::is_enum_v<std::decay_t<T>> ||
                   is_cstring<T>::value ||
                   is_translation<T>::value ||
                   is_string_id<T>::value,
                   "Unsupported argument type" );
    throw_error( sf, "Tried to convert argument of type " +
                 std::string( demangle( typeid( T ).name() ) ) + " to " +
                 std::string( demangle( typeid( RT ).name() ) ) + ", which is not possible" );
}
/**@}*/

/**
 * Type-safe and undefined-behavior free wrapper over `sprintf`.
 * See @ref string_format for usage.
 * Basically it extracts the format specifiers and calls `sprintf` for each one separately
 * and with proper conversion of the input type.
 * For example `printf("%f", 7)` would yield undefined behavior as "%f" requires a `double`
 * as argument. This class detects the format specifier and converts the input to `double`
 * before calling `sprintf`. Similar for `printf("%d", "foo")` (yields UB again), but this
 * class will just throw an exception.
 */
// Note: argument index is always 0-based *in this code*, but `printf` has 1-based arguments.
class string_formatter
{
    private:
        /// Complete format string, including all format specifiers (the string passed
        /// to @ref printf).
        const std::string_view format;
        /// Used during parsing to denote the *next* character in @ref format to be
        /// parsed.
        size_t current_index_in_format = 0;
        /// The formatted output string, filled during parsing of @ref format,
        /// so it's only valid after the parsing has completed.
        std::string output;
        /// The *currently parsed format specifiers. This is extracted from @ref format
        /// during parsing and given to @ref sprintf (along with the actual argument).
        /// It is filled and reset during parsing for each format specifier in @ref format.
        std::string current_format;
        /// The *index* (not number) of the next argument to be formatted via @ref current_format.
        int current_argument_index = 0;
        /// Return the next character from @ref format and increment @ref current_index_in_format.
        /// Returns a null-character when the end of the @ref format has been reached (and does not
        /// change @ref current_index_in_format).
        char consume_next_input();
        /// Returns (like @ref consume_next_input) the next character from @ref format, but
        /// does *not* change @ref current_index_in_format.
        char get_current_input() const;
        /// If the next character to read from @ref format is the given character, consume it
        /// (like @ref consume_next_input) and return `true`. Otherwise don't do anything at all
        /// and return `false`.
        bool consume_next_input_if( char c );
        /// Return whether @ref get_current_input has a decimal digit ('0'...'9').
        bool has_digit() const;
        /// Consume decimal digits, interpret them as integer and return it.
        /// A starting '0' is allowed. Leaves @ref format at the first non-digit
        /// character (or the end). Returns 0 if the first character is not a digit.
        int parse_integer();
        /// Read and consume format flag characters and append them to @ref current_format.
        /// Leaves @ref format at the first character that is not a flag (or the end).
        void read_flags();
        /// Read and forward to @ref current_format any width specifier from @ref format.
        /// Returns nothing if the width is not specified or if it is specified as fixed number,
        /// otherwise returns the index of the printf-argument to be used for the width.
        std::optional<int> read_width();
        /// See @ref read_width. This does the same, but for the precision specifier.
        std::optional<int> read_precision();
        /// Read and return the index of the printf-argument that is to be formatted. Returns
        /// nothing if @ref format does not refer to a specific index (caller should use
        /// @ref current_argument_index).
        std::optional<int> read_argument_index();
        // Helper for common logic in @ref read_width and @ref read_precision.
        std::optional<int> read_number_or_argument_index();
        /// Throws an exception containing the given message and the @ref format.
        [[noreturn]]
        void throw_error( const std::string &msg ) const;
        friend void throw_error( const string_formatter &sf, const std::string &msg ) {
            sf.throw_error( msg );
        }
        mutable std::string temp_buffer;
        /// Stores the given text in @ref temp_buffer and returns `c_str()` of it. This is used
        /// for printing non-strings through "%s". It *only* works because this prints each format
        /// specifier separately, so the content of @ref temp_buffer is only used once.
        friend std::string_view string_formatter_set_temp_buffer( const string_formatter &sf,
                std::string_view text ) {
            sf.temp_buffer = text;
            return std::string_view( sf.temp_buffer );
        }
        /**
         * Extracts a printf argument from the argument list and converts it to the requested type.
         * @tparam RT The type that the argument should be converted to.
         * @tparam current_index The index of the first of the supplied arguments.
         * @throws If there is no argument with the given index, or if the argument can not be
         * converted to the requested type (via @ref convert).
         */
        /**@{*/
        template<typename RT, unsigned int current_index>
        RT get_nth_arg_as( const unsigned int requested ) const {
            throw_error( "Requested argument " + std::to_string( requested ) +
                         " but input has only " + std::to_string( current_index )
                       );
        }
        template<typename RT, unsigned int current_index, typename T, typename ...Args>
        RT get_nth_arg_as( const unsigned int requested, T &&head, Args &&... args ) const {
            if( requested > current_index ) {
                return get_nth_arg_as < RT, current_index + 1 > ( requested, std::forward<Args>( args )... );
            } else {
                return convert( static_cast<RT *>( nullptr ), *this, std::forward<T>( head ), 0 );
            }
        }
        /**@}*/

        void add_long_long_length_modifier();
        void discard_oct_hex_sign_flag();

        template<typename ...Args>
        void read_conversion( const int format_arg_index, Args &&... args ) {
            // Removes the prefix "ll", "l", "h" and "hh", "z", and "t".
            // We later add "ll" again and that
            // would interfere with the existing prefix. We convert *all* input to (un)signed
            // long long int and use the "ll" modifier all the time. This will print the
            // expected value all the time, even when the original modifier did not match.
            if( consume_next_input_if( 'l' ) ) {
                consume_next_input_if( 'l' );
            } else if( consume_next_input_if( 'h' ) ) {
                consume_next_input_if( 'h' );
            } else if( consume_next_input_if( 'z' ) ) {
                // done with it
            } else if( consume_next_input_if( 't' ) ) {
                // done with it
            }
            const char c = consume_next_input();
            current_format.push_back( c );
            switch( c ) {
                case 'c':
                    return do_formating( get_nth_arg_as<int, 0>( format_arg_index, std::forward<Args>( args )... ) );
                case 'd':
                case 'i':
                    add_long_long_length_modifier();
                    return do_formating( get_nth_arg_as<signed long long int, 0>( format_arg_index,
                                         std::forward<Args>( args )... ) );
                case 'o':
                case 'x':
                case 'X':
                    // Workaround for fmtlib prepending number with ' '/'+'
                    // when formatting with ' '/'+' flags and 'o'/'x'/'X' specifiers
                    discard_oct_hex_sign_flag();
                // intentional fall-through
                case 'u':
                    add_long_long_length_modifier();
                    return do_formating( get_nth_arg_as<unsigned long long int, 0>( format_arg_index,
                                         std::forward<Args>( args )... ) );
                case 'a':
                case 'A':
                case 'g':
                case 'G':
                case 'f':
                case 'F':
                case 'e':
                case 'E':
                    return do_formating( get_nth_arg_as<double, 0>( format_arg_index, std::forward<Args>( args )... ) );
                case 'p':
                    return do_formating( get_nth_arg_as<void *, 0>( format_arg_index,
                                         std::forward<Args>( args )... ) );
                case 's':
                    return do_formating( get_nth_arg_as<std::string_view, 0>( format_arg_index,
                                         std::forward<Args>( args )... ) );
                default:
                    throw_error( "Unsupported format conversion: " + std::string( 1, c ) );
            }
        }

        void do_formating( int value );
        void do_formating( signed long long int value );
        void do_formating( unsigned long long int value );
        void do_formating( double value );
        void do_formating( void *value );
        void do_formating( std::string_view value );

    public:
        /// @param format The format string as required by `sprintf`.
        string_formatter( std::string_view format ) : format( format ) { }
        /// Does the actual `sprintf`. It uses @ref format and puts the formatted
        /// string into @ref output.
        /// Note: use @ref get_output to get the formatted string after a successful
        /// call to this function.
        /// @throws Exceptions when the arguments do not match the format specifiers,
        /// see @ref get_nth_arg_as, or when the format is invalid for whatever reason.
        /// Note: @ref string_format is a wrapper that handles those exceptions.
        template<typename ...Args>
        void parse( Args &&... args ) {
            output.reserve( format.size() );
            output.resize( 0 );
            current_index_in_format = 0;
            current_argument_index = 0;
            while( const char c = consume_next_input() ) {
                if( c != '%' ) {
                    output.push_back( c );
                    continue;
                }
                if( consume_next_input_if( '%' ) ) {
                    output.push_back( '%' );
                    continue;
                }
                current_format = "%";
                const std::optional<int> format_arg_index = read_argument_index();
                read_flags();
                if( const std::optional<int> width_argument_index = read_width() ) {
                    const int w = get_nth_arg_as<int, 0>( *width_argument_index, std::forward<Args>( args )... );
                    current_format += std::to_string( w );
                }
                if( const std::optional<int> precision_argument_index = read_precision() ) {
                    const int p = get_nth_arg_as<int, 0>( *precision_argument_index, std::forward<Args>( args )... );
                    current_format += std::to_string( p );
                }
                const int arg = format_arg_index ? *format_arg_index : current_argument_index++;
                read_conversion( arg, std::forward<Args>( args )... );
            }
        }
        std::string get_output() const {
            return output;
        }
};

} // namespace cata

/**
 * Simple wrapper over @ref string_formatter::parse. It catches any exceptions and returns
 * some error string. Otherwise it just returns the formatted string.
 *
 * These functions perform string formatting according to the rules of the `printf` function,
 * see `man 3 printf` or any other documentation.
 *
 * In short: the \p format parameter is a string with optional placeholders, which will be
 * replaced with formatted data from the further arguments. The further arguments must have
 * a type that matches the type expected by the placeholder.
 * The placeholders look like this:
 * - `%s` expects an argument of type `const char*`, `std::string`, `std::string_view` or
 *   numeric (which is converted to a string via `std::to_string`), which is inserted as is.
 * - `%d` expects an argument of an integer type (int, short, ...), which is formatted as
 *   decimal number.
 * - `%f` expects a numeric argument (integer / floating point), which is formatted as
 *   decimal number.
 *
 * There are more placeholders and options to them (see documentation of `printf`).
 * Note that this wrapper (via @ref string_formatter) automatically converts the arguments
 * to match the given format specifier (if possible) - see @ref string_formatter_convert.
 */
/**@{*/
template<typename ...Args>
inline std::string string_format( std::string_view format, Args &&...args )
{
    try {
        cata::string_formatter formatter( format );
        formatter.parse( std::forward<Args>( args )... );
        return formatter.get_output();
    } catch( ... ) {
        return cata::handle_string_format_error();
    }
}
template<typename T, typename ...Args>
inline std::string
string_format( T &&format, Args &&...args )
requires cata::is_translation<T>::value {
    return string_format( format.translated(), std::forward<Args>( args )... );
}
/**@}*/

/** Print string to stdout. */
void cata_print_stdout( const std::string &s );
/** Print string to stderr. */
void cata_print_stderr( const std::string &s );

/** Same as @ref string_format, but prints its result to stdout. */
/**@{*/
template<typename ...Args>
inline void cata_printf( std::string_view format, Args &&...args )
{
    std::string s = string_format( format, std::forward<Args>( args )... );
    cata_print_stdout( s );
}

/**@}*/


