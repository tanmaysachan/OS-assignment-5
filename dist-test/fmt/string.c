6750 #include "types.h"
6751 #include "x86.h"
6752 
6753 void*
6754 memset(void *dst, int c, uint n)
6755 {
6756   if ((int)dst%4 == 0 && n%4 == 0){
6757     c &= 0xFF;
6758     stosl(dst, (c<<24)|(c<<16)|(c<<8)|c, n/4);
6759   } else
6760     stosb(dst, c, n);
6761   return dst;
6762 }
6763 
6764 int
6765 memcmp(const void *v1, const void *v2, uint n)
6766 {
6767   const uchar *s1, *s2;
6768 
6769   s1 = v1;
6770   s2 = v2;
6771   while(n-- > 0){
6772     if(*s1 != *s2)
6773       return *s1 - *s2;
6774     s1++, s2++;
6775   }
6776 
6777   return 0;
6778 }
6779 
6780 void*
6781 memmove(void *dst, const void *src, uint n)
6782 {
6783   const char *s;
6784   char *d;
6785 
6786   s = src;
6787   d = dst;
6788   if(s < d && s + n > d){
6789     s += n;
6790     d += n;
6791     while(n-- > 0)
6792       *--d = *--s;
6793   } else
6794     while(n-- > 0)
6795       *d++ = *s++;
6796 
6797   return dst;
6798 }
6799 
6800 // memcpy exists to placate GCC.  Use memmove.
6801 void*
6802 memcpy(void *dst, const void *src, uint n)
6803 {
6804   return memmove(dst, src, n);
6805 }
6806 
6807 int
6808 strncmp(const char *p, const char *q, uint n)
6809 {
6810   while(n > 0 && *p && *p == *q)
6811     n--, p++, q++;
6812   if(n == 0)
6813     return 0;
6814   return (uchar)*p - (uchar)*q;
6815 }
6816 
6817 char*
6818 strncpy(char *s, const char *t, int n)
6819 {
6820   char *os;
6821 
6822   os = s;
6823   while(n-- > 0 && (*s++ = *t++) != 0)
6824     ;
6825   while(n-- > 0)
6826     *s++ = 0;
6827   return os;
6828 }
6829 
6830 // Like strncpy but guaranteed to NUL-terminate.
6831 char*
6832 safestrcpy(char *s, const char *t, int n)
6833 {
6834   char *os;
6835 
6836   os = s;
6837   if(n <= 0)
6838     return os;
6839   while(--n > 0 && (*s++ = *t++) != 0)
6840     ;
6841   *s = 0;
6842   return os;
6843 }
6844 
6845 
6846 
6847 
6848 
6849 
6850 int
6851 strlen(const char *s)
6852 {
6853   int n;
6854 
6855   for(n = 0; s[n]; n++)
6856     ;
6857   return n;
6858 }
6859 
6860 
6861 
6862 
6863 
6864 
6865 
6866 
6867 
6868 
6869 
6870 
6871 
6872 
6873 
6874 
6875 
6876 
6877 
6878 
6879 
6880 
6881 
6882 
6883 
6884 
6885 
6886 
6887 
6888 
6889 
6890 
6891 
6892 
6893 
6894 
6895 
6896 
6897 
6898 
6899 
