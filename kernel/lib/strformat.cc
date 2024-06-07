#include <lib/strformat.hh>

enum {
    LengthDefault,
    LengthHh,
    LengthH,
    LengthL,
    LengthLl,
    LengthJ,
    LengthZ,
    LengthT,
    LengthLd
};

void PrintfWithCallbackV(void (*print_func)(char), const char *format, va_list params)
{
    char buffer[128];
    int total_written;

    #define PRINT_BUF do { char *start = buffer; while (*start) PRINT_CHAR(*start++); } while(0)
    #define PRINT_CHAR(x) do { print_func((x)); total_written++; } while(0)

    while (*format) {
        if (*format == '%') {
            format++;

            if (*format == '%') {
                PRINT_CHAR('%');
                format++;
            }
            else {
                /* Flags section */
                bool left_justify = false;
                bool print_sign = false;
                bool plus_to_space = false;
                bool write_prefix = false;
                bool pad_zeroes = false;
                int min_width = 0;
                int precision_digits = 0;

                #define FLAG_DEF(char, flag_var) \
                    if (*format == (char)) flag_var = true, format++

                while (*format) {
                    FLAG_DEF('-', left_justify);
                    else FLAG_DEF('+', print_sign);
                    else FLAG_DEF(' ', plus_to_space);
                    else FLAG_DEF('#', write_prefix);
                    else FLAG_DEF('0', pad_zeroes);
                    else break;
                }
                #undef FLAG_DEF

                /* Width section */
                if (!*format) break;
                int _;
                if (*format >= '0' && *format <= '9')
                    format = ParseUint(format, &min_width, &_);
                else if (*format == '*' && format++)
                    min_width = va_arg(params, int);

                if (!*format) break;

                /* Precision section */
                if (*format == '.' && format++) {
                    if (*format >= '0' && *format <= '9')
                        format = ParseUint(format, &precision_digits, &_);
                    else if (*format == '*' && format++)
                        min_width = va_arg(params, int);
                    if (!*format) break;
                }

                if (precision_digits > min_width)
                    min_width = precision_digits;

                int length = 0;

                if (format[0] == 'h' && format[1] == 'h')
                    length = LengthHh, format += 2;
                else if (format[0] == 'l' && format[1] == 'l')
                    length = LengthLl, format += 2;
                else if (format[0] == 'h')
                    length = LengthH, format++;
                else if (format[0] == 'l')
                    length = LengthL, format++;
                else if (format[0] == 'j')
                    length = LengthJ, format++;
                else if (format[0] == 'z')
                    length = LengthZ, format++;
                else if (format[0] == 't')
                    length = LengthT, format++;
                else if (format[0] == 'L')
                    length = LengthLd, format++;

                char specifier = *format++;

                switch (specifier) {
                case 'd': case 'i': case 'u':
                case 'o': case 'x': case 'X': {
                    u64 val;
                    int base =
                        specifier == 'o' ? 8 :
                        specifier == 'x' || specifier == 'X' ? 16 :
                        10;

                    int sgned = specifier == 'i' || specifier == 'd';

                    switch (length) {
                    case LengthHh:
                        val = (signed char)va_arg(params, int);
                        break;
                    case LengthH:
                        val = (short int)va_arg(params, int);
                        break;
                    case LengthL:
                        val = (long int)va_arg(params, int);
                        break;
                    case LengthLl:
                        val = va_arg(params, long long int);
                        break;
                    case LengthJ:
                        val = va_arg(params, intmax_t);
                        break;
                    case LengthZ:
                        val = va_arg(params, size_t);
                        break;
                    case LengthT:
                        val = va_arg(params, ptrdiff_t);
                        break;
                    default:
                        val = va_arg(params, int);
                        break;
                    }

                    if (sgned) {
                        if (val >= 0) {
                            if (print_sign) PRINT_CHAR('+');
                            else if (plus_to_space) PRINT_CHAR(' ');
                        }

                        IntToString<10, u64>(val, buffer, min_width,
                            pad_zeroes * '0', left_justify);

                        PRINT_BUF;
                    }
                    else {
                        if (print_sign) PRINT_CHAR('+');
                        else if (plus_to_space) PRINT_CHAR(' ');

                        if (base == 8) {
                            if (write_prefix) PRINT_CHAR('0');
                            IntToString<8, u64>(val, buffer, min_width,
                                pad_zeroes * '0', left_justify);
                            PRINT_BUF;
                        }

                        if (base == 10) {
                            IntToString<10, u64>(val, buffer, min_width,
                                pad_zeroes * '0', left_justify);
                            PRINT_BUF;
                        }

                        if (base == 16) {
                            if (write_prefix) {
                                PRINT_CHAR('0');
                                // x or X
                                PRINT_CHAR(specifier);
                            }

                            IntToString<16, u64>(val, buffer, min_width,
                                pad_zeroes * '0', left_justify,
                                specifier == 'X');
                            PRINT_BUF;
                        }
                    }
                    break;
                }

                case 'c': {
                    int pad = min_width - 1;
                    if (!left_justify)
                        for (int i = 0; pad > i; i++)
                            PRINT_CHAR(pad_zeroes ? '0' : ' ');
                    PRINT_CHAR(va_arg(params, int));
                    if (left_justify)
                        for (int i = 0; pad > i; i++)
                            PRINT_CHAR(pad_zeroes ? '0' : ' ');
                    break;
                }

                case 's':
                    #define PRINT_STR_PADDED(width) \
                        do { \
                            width *str = va_arg(params, width*); \
                            int pad = min_width - StringLength<width>(str); \
                            if (!left_justify) \
                                for (int i = 0; pad > i; i++) \
                                    PRINT_CHAR(pad_zeroes ? '0' : ' '); \
                            while (*str) PRINT_CHAR(*str++); \
                            if (left_justify) \
                                for (int i = 0; pad > i; i++) \
                                    PRINT_CHAR(pad_zeroes ? '0' : ' '); \
                        } while (0)

                    if (length == LengthL)
                        PRINT_STR_PADDED(wchar_t);
                    else
                        PRINT_STR_PADDED(char);

                    #undef PRINT_STR_PADDED
                    break;

                case 'p':
                case 'P':
                    IntToString<16, uint64_t>((uint64_t)(uintptr_t)va_arg(params, void *), buffer,
                        sizeof(void *) * 2, '0', 0, specifier == 'P');
                    PRINT_BUF;
                    break;

                case 'f':
                case 'F': {
                    long double dbl =
                        length == LengthLd ?
                        va_arg(params, long double) :
                        va_arg(params, double);
                    if (dbl >= 0) {
                        if (print_sign) PRINT_CHAR('+');
                        else if (plus_to_space) PRINT_CHAR(' ');
                    }
                    FloatToString<10>(dbl, buffer, precision_digits, min_width,
                        pad_zeroes * '0', left_justify, specifier == 'F');
                    PRINT_BUF;
                    break;
                }

                case 'n':
                    switch (length) {
                    case LengthHh:
                        *va_arg(params, signed char *) = total_written;
                        break;
                    case LengthH:
                        *va_arg(params, short int *) = total_written;
                        break;
                    case LengthL:
                        *va_arg(params, long int *) = total_written;
                        break;
                    case LengthLl:
                        *va_arg(params, long long int *) = total_written;
                        break;
                    case LengthJ:
                        *va_arg(params, intmax_t *) = total_written;
                        break;
                    case LengthZ:
                        *va_arg(params, size_t *) = total_written;
                        break;
                    case LengthT:
                        *va_arg(params, ptrdiff_t *) = total_written;
                        break;
                    default:
                        *va_arg(params, int *) = total_written;
                        break;
                    }
                    break;
                }
            }
        }
        else
            PRINT_CHAR(*format++);
    }
}