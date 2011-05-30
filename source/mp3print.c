/*

  Implementation of functions mp3printf()
  Used to print a mp3 formatted string by Colten Edwards.
  Based on snprintf() from ssh.
  $Id: mp3print.c,v 1.1.1.1 2000/08/04 08:58:29 edwards Exp $
  */
#include "teknap.h"
#include "struct.h"
#include "ircaux.h"
#include "napster.h"

#undef isdigit
#define isdigit(ch) ((ch) >= '0' && (ch) <= '9')

#define MINUS_FLAG 0x1
#define PLUS_FLAG 0x2
#define SPACE_FLAG 0x4
#define HASH_FLAG 0x8
#define CONV_TO_SHORT 0x10
#define IS_LONG_INT 0x20
#define IS_LONG_DOUBLE 0x40
#define X_UPCASE 0x80
#define IS_NEGATIVE 0x100
#define UNSIGNED_DEC 0x200
#define ZERO_PADDING 0x400
#define COMMA_PADDING 0x800
#define SIZE_PADDING 0x1000

#undef sprintf

/* Extract a formatting directive from str. Str must point to a '%'. 
   Returns number of characters used or zero if extraction failed. */

static int
snprintf_get_directive(const char *str, int *flags, int *width,
                       int *precision, char *format_char)
{
int length, value;
const char *orig_str = str;

	*flags = 0; 
	*width = 0;
	*precision = 0;
	*format_char = (char)0;

	if (*str == '%')
	{
		/* Get the flags */
		str++;
		while (*str == '-' || *str == '+' || *str == ' ' 
			|| *str == '#' || *str == '0' || *str == ',' || *str == 'g')
		{
			switch (*str)
			{
				case '-':
					*flags |= MINUS_FLAG;
					break;
				case '+':
					*flags |= PLUS_FLAG;
					break;
				case ' ':
					*flags |= SPACE_FLAG;
					break;
				case '#':
					*flags |= HASH_FLAG;
					break;
				case '0':
					*flags |= ZERO_PADDING;
					break;
				case ',':
					*flags |= COMMA_PADDING;
					break;
				case 'g':
					*flags |= SIZE_PADDING;
					break;
			}
			str++;
		}

		/* Don't pad left-justified numbers withs zeros */
		if ((*flags & MINUS_FLAG) && (*flags & ZERO_PADDING))
			*flags &= ~ZERO_PADDING;
        
		/* Is width field present? */
		if (isdigit(*str))
		{
			for (value = 0; *str && isdigit(*str); str++)
				value = 10 * value + *str - '0';
			*width = value;
		}
		/* Is the precision field present? */
		if (*str == '.')
		{
			str++;
			if (isdigit(*str))
			{
				for (value = 0; *str && isdigit(*str); str++)
					value = 10 * value + *str - '0';
				*precision = value;
			}
			else
				*precision = 0;
		}
		/* Get and check the formatting character */

		*format_char = *str;
		str++;
		length = str - orig_str;

		switch (*format_char)
		{
			case 'b': case 'l': case 'L': case 't': case 'T': 
			case 'd': case 'f': case 'F': case 'h': case 'H': 
			case 'm': case 'M': case 'N': case 's': case 'S': 
			case 'a': case 'p':
				return length;

			default:
				return 0;
		}
	}
	return 0;
}

/* Convert a integer from unsigned long int representation 
   to string representation. This will insert prefixes if needed 
   (leading zero for octal and 0x or 0X for hexadecimal) and
   will write at most buf_size characters to buffer.
   tmp_buf is used because we want to get correctly truncated
   results.
   */

static int
snprintf_convert_ulong(char *buffer, size_t buf_size, int base, char *digits,
                       unsigned long int ulong_val, int flags, int width,
                       int precision)
{
int tmp_buf_len = 100 + width * 2, len = 0;

char *tmp_buf, *tmp_buf_ptr, prefix[2];

	tmp_buf = alloca(tmp_buf_len+1);

	prefix[0] = '\0';
	prefix[1] = '\0';
  
	/* Make tmp_buf_ptr point just past the last char of buffer */
	tmp_buf_ptr = tmp_buf + tmp_buf_len;
	/* Main conversion loop */
	do 
	{
		*--tmp_buf_ptr = digits[ulong_val % base];
		ulong_val /= base;
		len++;
		if (flags & COMMA_PADDING && !(len % 3))
			*--tmp_buf_ptr = ',';
		precision--;
	} 
	while ((ulong_val != 0 || precision > 0) && tmp_buf_ptr > tmp_buf);
 
	/* Get the prefix */
	if (!(flags & IS_NEGATIVE))
	{
		if (base == 16 && (flags & HASH_FLAG))
		{
			if (flags && X_UPCASE)
			{
				prefix[0] = 'x';
				prefix[1] = '0';
			}
			else
			{
				prefix[0] = 'X';
				prefix[1] = '0';
			}
		}
		if (base == 8 && (flags & HASH_FLAG))
			prefix[0] = '0';
     
		if (base == 10 && !(flags & UNSIGNED_DEC) && (flags & PLUS_FLAG))
			prefix[0] = '+';
		else if (base == 10 && !(flags & UNSIGNED_DEC) && (flags & SPACE_FLAG))
			prefix[0] = ' ';
	}
	else
		prefix[0] = '-';

	if (prefix[0] != '\0' && tmp_buf_ptr > tmp_buf)
	{
		*--tmp_buf_ptr = prefix[0];
		if (prefix[1] != '\0' && tmp_buf_ptr > tmp_buf)
			*--tmp_buf_ptr = prefix[1];
	}
  
	len = (tmp_buf + tmp_buf_len) - tmp_buf_ptr;

	if (len <= buf_size)
	{
		if (len < width)
		{
			if (width > (tmp_buf_ptr - tmp_buf))
				width = (tmp_buf_ptr - tmp_buf);
			if (flags & MINUS_FLAG)
			{
				memcpy(buffer, tmp_buf_ptr, len);
				memset(buffer + len, (flags & ZERO_PADDING)?'0':' ',
					width - len);
				len = width;
			}
			else
			{
				memset(buffer, (flags & ZERO_PADDING)?'0':' ',
					width - len);
				memcpy(buffer + width - len, tmp_buf_ptr, len);
				len = width;
			}
		}
		else
			memcpy(buffer, tmp_buf_ptr, len);
		return len;
	}
	else
	{
		memcpy(buffer, tmp_buf_ptr, buf_size);
		return buf_size;
	}
}

static int
snprintf_convert_float(char *buffer, size_t buf_size,
                       double dbl_val, double orig_val, int flags, int width,
                       int precision, char format_char)
{
char print_buf[160], print_buf_len = 0;
char format_str[80], *format_str_ptr;

	format_str_ptr = format_str;
	*format_str_ptr = 0;
	if (width > 155) width = 155;
	if (precision <= 0)
		precision = 6;
	if (precision > 120)
		precision = 120;

  /* Construct the formatting string and let system's sprintf
     do the real work. */
  
	*format_str_ptr++ = '%';

	if (flags & MINUS_FLAG)
		*format_str_ptr++ = '-';
	if (flags & PLUS_FLAG)
		*format_str_ptr++ = '+';
	if (flags & SPACE_FLAG)
		*format_str_ptr++ = ' ';
	if (flags & ZERO_PADDING)
		*format_str_ptr++ = '0';
	if (flags & HASH_FLAG)
		*format_str_ptr++ = '#';
	sprintf(format_str_ptr, "%d.%d", width, precision);
	format_str_ptr += strlen(format_str_ptr);

	if (flags & IS_LONG_DOUBLE)
		*format_str_ptr++ = 'L';
    
	*format_str_ptr++ = 'f';
/*  *format_str_ptr++ = format_char;*/
	*format_str_ptr++ = '\0';

	sprintf(print_buf, format_str, dbl_val);
	if (flags & SIZE_PADDING)
		strlcat(print_buf, _GMKs(orig_val), buf_size);
	print_buf_len = strlen(print_buf);

	if (print_buf_len > buf_size)
		print_buf_len = buf_size;
	strncpy(buffer, print_buf, print_buf_len);
	return print_buf_len;
}

static int convert_string_value(char *str_val, char *str, int left, int flags, int width, int precision)
{
int i;
	if (str_val == NULL)
		str_val = "(null)";
	if (precision == 0)
		precision = strlen(str_val);
	else
	{
		if (memchr(str_val, 0, precision) != NULL)
			precision = strlen(str_val);
	}
	if (precision > left)
		precision = left;
	if (width > left)
		width = left;
	if (width < precision)
		width = precision;
	i = width - precision;

	if (flags & MINUS_FLAG)
	{
		strncpy(str, str_val, precision);
		memset(str + precision,
			(flags & ZERO_PADDING)?'0':' ', i);
	}
	else
	{
		memset(str, (flags & ZERO_PADDING)?'0':' ', i);
		strncpy(str + i, str_val, precision);
	}
	return width;
}


static int convert_dir_value(char **dir_val, char *str, int left, int flags, int dir_count, int width)
{
int i, len = 0, last_len = 0;
char *new_str, *ptr;
	for (i = 0; dir_val[i]; i++)
	{
		last_len = strlen(dir_val[i]);
		len += last_len;
	}
	new_str = alloca(len + dir_count + 10);
	memset(new_str, 0, len + dir_count + 5);
	ptr = new_str + len - last_len - 1;
		
	if (!dir_val[0])
	{
		dir_val[0] = "";
		*dir_val[1] = 0;
	}
	if (width > dir_count)
		width = dir_count;
	
	if (flags & MINUS_FLAG)
	{
		for (i = dir_count - 1; dir_val[i]; i--)
		{
			*(dir_val[i]-1) = '/';
			ptr = dir_val[i]-1;
			width--;
			if (!width)
				break;
		}
		strncpy(str, ptr, left);
		width = strlen(ptr);
	}
	else
	{
		for (i = 0; dir_val[i]; i++)
		{
			strcat(new_str, "/");
			strcat(new_str, dir_val[i]);
			width--;
			if (!width)
				break;
		}
		strncpy(str, new_str, left);
		width = strlen(new_str);
	}
	return width;
}


extern int mp3printf(char *str, size_t size, const char *format, FileStruct *fp)
{
  int status, left = (int)size - 1;
  const char *format_ptr = format;
  int flags, width, precision;
  char format_char;
  long int long_val;
  unsigned long int ulong_val;
  char *str_val;
  char *filename;
  char *dirname;
  double dbl_val;
  char *dir_array[20];
  int dir_count = 0;
  
	str_val = LOCAL_COPY(fp->filename);
	if ((filename = strrchr(str_val, '/')) || (filename = strrchr(str_val, '\\')))
		*filename++ = 0;
	dirname = str_val;
	flags = 0;
        for (dirname = str_val; *dirname && flags < 14; dirname++)
        {
        	if ((*dirname == '/') || (*dirname == '\\'))
        	{
        		*dirname++ = 0;
        		dir_array[flags++] = dirname;
			dir_count++;
        	}
        }
        dir_array[flags] = 0;
	flags = 0;
	while (format_ptr < format + strlen(format))
	{
		if (*format_ptr == '%')
		{
			if (format_ptr[1] == '%' && left > 0)
			{
				*str++ = '%';
				left--;
				format_ptr += 2;
			}
			else
			{
				if (left <= 0)
				{
					*str = '\0';
					return size;
				}
				else
				{
					 status = snprintf_get_directive(format_ptr, 
					 	&flags, &width,
						&precision, &format_char); 

					if (status == 0)
					{
						*str = '\0';
						return 0;
					}
					else
					{
			format_ptr += status;
			/* Print a formatted argument */
			switch (format_char)
			{
				case 'b':
					if (flags & IS_LONG_INT)
						long_val = fp->bitrate;
					else
						long_val = (long int) fp->bitrate;
                          
					if (long_val < 0)
					{
						ulong_val = (unsigned long int) -long_val;
						flags |= IS_NEGATIVE;
					}
					else
						ulong_val = (unsigned long int) long_val;

					status = snprintf_convert_ulong(str, 
						left, 10, "0123456789",
						ulong_val, flags,
						width, precision);
					str += status;
					left -= status;
					break;

				case 'l':
					ulong_val = fp->filesize;
					status = snprintf_convert_ulong(str, 
					left, 10, "0123456789",
					ulong_val, flags, width, precision);
					str += status;
					left -= status;
					break;

				case 'L':
					dbl_val = (double) _GMKv(fp->filesize);
					status = snprintf_convert_float(str, 
						left, dbl_val, fp->filesize, 
						flags, width, precision,format_char);
					str += status;
					left -= status;
					break;
				case 'p':
					if (fp->result.tv_sec)
					{
						dbl_val = (double) time_diff(fp->start, fp->result);
						status = snprintf_convert_float(str, 
							left, dbl_val, time_diff(fp->start, fp->result), 
							flags, width, precision, format_char);
					}
					else
					{
						strcpy(str, "N/A");
						status = 3;
					}
					str += status;
					left -= status;
					break;
				case 'a':
					width = convert_string_value(_GMKs(fp->filesize),
						 str, left, flags, width, precision);
					str += status;
					left -= status;
					break;			
				case 't':
					width = convert_string_value(print_time(fp->seconds), 
						str, left, flags, width, precision);
					str += width;
					left -= width;
					break;
				case 'M':
					width = convert_string_value(fp->checksum, 
						str, left, flags, width, precision);
					str += width;
					left -= width;
					break;
				case 'm':
					width = convert_string_value(find_mime_type(fp->filename), 
						str, left, flags, width, precision);
					str += width;
					left -= width;
					break;
				case 'N':
					str_val = fp->nick;
					width = convert_string_value(str_val, 
						str, left, flags, width, precision);
					str += width;
					left -= width;
					break;

				case 'S':
					width = convert_string_value(mode_str(fp->stereo), 
						str, left, flags, width, precision);
					str += width;
					left -= width;
					break;
				case 'T':
					ulong_val = fp->seconds;
					status = snprintf_convert_ulong(str, 
						left, 10, "0123456789",
						ulong_val, flags, width, precision);
					str += status;
					left -= status;
					break;
				case 'd':
					str_val = dirname;
					width = convert_dir_value(dir_array, 
						str, left, flags, dir_count, width);
					str += width;
					left -= width;
					break;
			
				case 'f': 
				case 'F':
					if (format_char == 'f')
						str_val = filename;
					else
						str_val = fp->filename;
					width = convert_string_value(str_val, 
						str, left, flags, width, precision);
					str += width;
					left -= width;
					break;
				case 'h':
					ulong_val = fp->freq;
					status = snprintf_convert_ulong(str, 
						left, 10, "0123456789",
						ulong_val, flags, width, precision);
					str += status;
					left -= status;
					break;
				case 'H':
					dbl_val = ((double) fp->freq) / 1000;
					status = snprintf_convert_float(str, 
						left, dbl_val, fp->freq, 
						flags, width, precision, format_char);
					str += status;
					left -= status;
					break;
				default:
					break;
			} /* switch */
					}
				}
			}
		}
		else
		{
			if (left > 0)
			{
				*str++ = *format_ptr++;
				left--;
			}
			else
			{
				*str = '\0';
				return size;
			}
		} /* if */
	} /* while */
	*str = '\0';
	return size - left - 1;
}


#ifdef TEST

int main(int c, char **argv)
{
FileStruct new = { 0 };
char buffer[2049];
	memset(buffer, 0, sizeof(buffer));
	new.filename = (char *)malloc(100);
	new.nick = (char *)malloc(100);
	new.checksum = (char *)malloc(100);
	strcpy(new.filename, "/this/is/a/filename of something.mp3");
	strcpy(new.nick, "qr1");
	strcpy(new.checksum, "CHECKSUMCHECKSUM");
	new.filesize = 8384834;
	new.bitrate = 160;
	new.freq = 44100;
	new.stereo = 1;
	new.type = 1;
	new.seconds = 800;
	mp3printf(buffer, sizeof(buffer)-1, 
		"%b \"%-30f\" %2.1H %3.1h [%t] %T %S %s %m %M %,l %g4.2L %3d/ %-2d/ %F", &new);
	fprintf(stderr, "%s\n", buffer);
}
#endif
