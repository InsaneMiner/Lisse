#define ITOA_HEX_VALUE 16
#define ITOA_DECIMAL_VALUE 10
#define ITOA_BINARY_VALUE 2



/*
itoa example
int i = 10;
char buffer [33];
itoa (i,buffer,ITOA_DECIMAL_VALUE);
printf ("decimal: %s\n",buffer);
itoa (i,buffer,ITOA_HEX_VALUE);
printf ("hexadecimal: %s\n",buffer);
itoa (i,buffer,ITOA_BINARY_VALUE);
printf ("binary: %s\n",buffer);
*/

//convert number to char *
char * itoa( int value, char * str, int base )
{
    char * rc;
    char * ptr;
    char * low;
    // Check for supported base.
    if ( base < 2 || base > 36 )
    {
        *str = '\0';
        return str;
    }
    rc = ptr = str;
    // Set '-' for negative decimals.
    if ( value < 0 && base == 10 )
    {
        *ptr++ = '-';
    }
    // Remember where the numbers start.
    low = ptr;
    // The actual conversion.
    do
    {
        // Modulo is negative for negative value. This trick makes abs() unnecessary.
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + value % base];
        value /= base;
    } while ( value );
    // Terminating the string.
    *ptr-- = '\0';
    // Invert the numbers.
    while ( low < ptr )
    {
        char tmp = *low;
        *low++ = *ptr;
        *ptr-- = tmp;
    }
    return rc;
}