#include "parlay/internal/block_allocator.h"
#include "parlay/primitives.h"
#include "parlay/io.h"
#include <string.h>

using uint = unsigned int;
using ushort = unsigned short;
using uchar = unsigned char;

//using Line = parlay::sequence<parlay::slice<char*, char*>>;
using Line = parlay::sequence<parlay::sequence<char>>;

//void copy_string(char* dest, parlay::slice<char*, char*> src, size_t max_len) {
void copy_string(char* dest, parlay::sequence<char> src, size_t max_len) {
  if (src.size() > max_len) {
    std::cout << "string too long while parsing tables" << std::endl;
    abort();
  }
  parlay::parallel_for(0, src.size(), [&] (size_t i) { dest[i] = src[i];});
  dest[src.size()] = 0;
}

//int str_to_int(parlay::slice<char*, char*> str) {
int str_to_int(parlay::sequence<char> const &str) {
  return parlay::chars_to_long(str);
}

int slice_to_int(parlay::slice<char*, char*> str) {
  return parlay::internal::chars_to_int_t<long>(str);
}

//double str_to_double(parlay::slice<char*, char*> str) {
double str_to_double(parlay::sequence<char> const &str) {
  return parlay::chars_to_double(str);
}

double slice_to_double(parlay::slice<char*, char*> str) {
  return parlay::chars_to_double(parlay::to_sequence(str));
}

struct Date {
  union as {
    uint c;
    struct {
      ushort day;
      uchar month;
      uchar year;
    } d;
  };
  as v;
  const static int start_year = 1900;

  static bool less(Date a, Date b) {
    return a.v.c < b.v.c;
  }
  
  static bool less_or_equal(Date a, Date b) {
    return a.v.c <= b.v.c;
  }
  
  int year() {return v.d.year + start_year;}
  int month() {return v.d.month;}
  int day() {return v.d.day;}

  template <class Str>
  void from_str(Str str) {
    v.d.year = slice_to_int(str.cut(0,4)) - start_year;
    v.d.month = slice_to_int(str.cut(5,7));
    v.d.day = slice_to_int(str.cut(8,10));}

  friend ostream &operator<<(ostream& stream, const Date& date) {
    stream << ((uint) date.v.d.year + start_year) << "-"
	   << ((uint) date.v.d.month) << "-" << date.v.d.day;
    return stream;
  }
  
  void add_one_day() {
    v.d.day++;
    if (v.d.day >28) {
      v.d.month++; v.d.day = 1;
      if (v.d.month >12) {
	v.d.year++; v.d.month = 1;
      }
    }
  }
  
  Date() = default;
  Date(int day, int month, int year) {
    v.d.year = year - start_year;
    v.d.month = month;
    v.d.day = day;}
  Date(const string x) { from_str(parlay::to_chars(x));}
  Date(char* str) { from_str(parlay::to_chars(str));}
  //Date(parlay::slice<char*, char*> str) { from_str(str);}
  Date(parlay::sequence<char> str) { from_str(str);}
  
  string to_str() {
    string y = std::to_string(year());
    string m = std::to_string(month());
    if (month() < 10) m = "0"+m;
    string d = std::to_string(day());
    string result = y + "-" + m + "-" + d;
    return result;
  }	  

};

const static Date infty_date = Date(0,0,2100);

// stores a percent from 1 to 100
struct Percent {
  uchar _val;
  Percent() {}
  //Percent(parlay::slice<char*, char*> str) : _val((uchar) (str_to_double(str)*100.0)) {}
  Percent(parlay::sequence<char> str) : _val((uchar) (str_to_double(str)*100.0)) {}
  double val() {return ((double) _val) * (double) .01;}
  Percent(int p) : _val(p) {}
};

// lineitem flags are 8bits: MMMIIRRL
// shipmode: M, shipinstruct: I, return flag: R, linestatus: LINESTATUES: L
// linestatus(&1): O:1, F:0
// return flag (&6): R:0, A:2, N:4
// shipinstruct (&24): NONE(N): 0, TAKE BACK RETURN(T): 8,
//                     DELIVER IN PERSON(D):16, COLLECT COD(C): 24
// shipmode (&224): RAIL(I): 0, AIR(A): 32, TRUCK(T): 64, FOB(F): 96,
//                  REG AIR(G): 128, MAIL(M): 160, SHIP(S): 192
//
struct lineitem_flags {
  uchar _flags;
  lineitem_flags() {}
  lineitem_flags(int flags) : _flags(flags) {}
  lineitem_flags(char linestatus, char returnflag, char shipinstruct,
		 char shipmode) {
    _flags = ((linestatus == 'O' ? 1:0)
	      | (returnflag == 'A' ? 2:0)
	      | (returnflag == 'N' ? 4:0)
	      | (shipinstruct == 'T' ? 8:0)
	      | (shipinstruct == 'D' ? 16:0)
	      | (shipinstruct == 'C' ? 24:0)
	      | (shipmode == 'A' ? 32:0)
	      | (shipmode == 'T' ? 64:0)
	      | (shipmode == 'F' ? 96:0)
	      | (shipmode == 'G' ? 128:0)
	      | (shipmode == 'M' ? 160:0)
	      | (shipmode == 'S' ? 192:0));
  }
  char linestatus () {
    return (_flags & 1) ? 'O' : 'F';}
  void set_linestatus() {_flags = _flags | 1;}
  char returnflag () {
    return ((_flags & 6) == 0) ? 'R' : ((_flags & 6) == 2) ? 'A' : 'N';
  }
  char shipinstruct() {
    uchar t = _flags & 24;
    if (t == 0) return 'N';
    if (t == 8) return 'T';
    if (t == 16) return 'D';
    if (t == 24) return 'C';
    return '?';
  }
  char shipmode() {
    uchar t = _flags & 224;
    if (t == 0) return 'I';
    if (t == 32) return 'A';
    if (t == 64) return 'T';
    if (t == 96) return 'F';
    if (t == 128) return 'G';
    if (t == 160) return 'M';
    if (t == 192) return 'S';
    return '?';
  }
  int as_int() { return _flags;}
  static constexpr size_t num = 6;
};

using dkey_t = uint;
using linenumber_t = uchar;
using quantity_t = uchar;
using price_t = float;

void check_length(Line const &S, size_t len) {
  if (S.size() != len) {
    cout << "inconsistent entry length, expected " << len
	 << " got " << S.size() << endl;
    abort();
  }
}

// size 40
struct Lineitem {
  dkey_t orderkey;
  dkey_t partkey;
  dkey_t suppkey;
  Date s_date;
  Date c_date;
  Date r_date;
  price_t e_price;
  linenumber_t linenumber;
  Percent discount;
  Percent tax;
  lineitem_flags flags;
  size_t strings;
  Lineitem() = default;
  Lineitem(Line const &S) {
    //orderkey = str_to_int(parlay::make_slice(S[0].begin(), S[0].end()));
    check_length(S,16);
    orderkey = str_to_int(S[0]);
    partkey = str_to_int(S[1]);
    suppkey = str_to_int(S[2]);
    linenumber = str_to_int(S[3]);
    uchar quantity = str_to_int(S[4]);
    e_price = str_to_double(S[5]);
    discount = Percent(S[6]);
    tax = Percent(S[7]);
    uchar returnflag = S[8][0];
    uchar linestatus = S[9][0];
    uchar si = S[13][0];
    uchar sm = S[14][0];
    if (sm == 'R') sm = S[14][2];
    flags = lineitem_flags(linestatus, returnflag, si, sm);
    s_date = Date(S[10]);
    c_date = Date(S[11]);
    r_date = Date(S[12]);
    //char* x = (char*) malloc(16); // pointer to strings
    char* x = 0;
    strings = quantity; 
    strings |= ((size_t) x) << 8; 
  }
  quantity_t quantity () {return strings & 255;}
  char shipinstruct() {return flags.shipinstruct();}
  char shipmode() {return flags.shipmode();}
  char returnflag() {return flags.returnflag();}
  char* comment() {return (char*) (strings >> 8) + 37;}
  void set_linestatus() {flags.set_linestatus();}
};

Lineitem* li;

parlay::internal::block_allocator order_str_pool(128); //96);

//size 32
struct Orders {
  dkey_t orderkey;
  dkey_t custkey;
  float totalprice;
  Date orderdate;
  int shippriority; 
  uchar orderpriority;
  char status;
  char* strings;
Orders() : strings(nullptr) {}
  Orders(Line const &S) {
    check_length(S, 9);
    orderkey = str_to_int(S[0]);
    custkey = str_to_int(S[1]);
    status = S[2][0];
    totalprice = str_to_double(S[3]);
    orderdate = Date(S[4]);
    orderpriority = S[5][0]-48;
    shippriority = str_to_int(S[7]);
    strings = (char*) order_str_pool.alloc();
    copy_string(strings, S[8], 79);  // comment
    copy_string(strings+80, S[6], 15); // clerk
  }
  //char* orderpriority() {return strings;}
  char* clerk() {return strings+80;}
  char* comment() {return strings;}
};

parlay::internal::block_allocator customer_str_pool(192);

//size 24
struct Customer {
  dkey_t custkey;
  dkey_t nationkey;
  float acctbal;
  char* strings;
  Customer() : strings(nullptr) {}
  Customer(Line const &S) {
    check_length(S, 8);
    custkey = str_to_int(S[0]);
    nationkey = str_to_int(S[3]);
    acctbal = str_to_double(S[5]);
    //cout << "dd" << endl;
    strings = (char*) customer_str_pool.alloc();
    //cout << "customer: " << custkey << endl;
    copy_string(strings, S[1], 25);
    copy_string(strings+26, S[2], 40);
    copy_string(strings+47, S[4], 15);
    copy_string(strings+63, S[6], 10);
    copy_string(strings+74, S[7], 117);
  }
  char* name() {return strings;} // S[1] // 25 long var
  char* address() {return strings + 26;} // S[2] // 40 long var
  char* phone() {return strings + 47;}  // S[4] // 15 long 
  char* mktsegment() {return strings + 63;} // S[6] // 10 long
  char* comment() {return strings + 74;} // S[7] // 117 long var // total 192
};

parlay::internal::block_allocator supp_str_pool(185);

//size 24
struct Supplier {
  dkey_t suppkey;
  char* strings;
  float acctbal;
  dkey_t nationkey;
  Supplier() : strings(nullptr) {}
  Supplier(Line const &S) {
    check_length(S, 7);
    suppkey = str_to_int(S[0]);
    nationkey = str_to_int(S[3]);
    acctbal = str_to_double(S[5]);
    //strings = (char*) pbbs::my_alloc(185);
    strings = (char*) supp_str_pool.alloc();
		
    copy_string(strings, S[1], 25); // name
    copy_string(strings+26, S[4], 15); //phone
    copy_string(strings+42, S[2], 40); //address
    copy_string(strings+83, S[6], 101); //address
		
  }
	
  char* name() {return strings;} // 25 chars, S[1]
  char* phone() {return strings+26;} // 15 digits, S[4]
  char* address() {return strings+42;} // 40 chars, S[2]
  char* comment() {return strings+83;} //101 chars, S[6]
};

struct Nations {
  dkey_t nationkey, regionkey;
  char* name;
  char* comment;
  Nations() {}
  Nations(Line const &S) {
    check_length(S, 4);
    nationkey = str_to_int(S[0]);
    name = new char[S[1].size()+1];
    copy_string(name,S[1],1000);
    regionkey = str_to_int(S[2]);
    comment = new char[S[3].size()+1];
    copy_string(comment,S[3],1000);
  }
};

parlay::sequence<Nations> nations;



parlay::internal::block_allocator part_str_pool(144);
//parlay::block_allocator part_str_pool(155);
//parlay::block_allocator part_str_pool(192);

//size 24
struct Part {
  dkey_t partkey;
  int size;
  float retailprice;
  char* strings;
  char brandstr[16];  // testing
  Part() : strings(nullptr) {}
  Part(Line const &S) {
    check_length(S, 9);
    partkey = str_to_int(S[0]);
    strings = (char*) part_str_pool.alloc();
    size = str_to_int(S[5]);
    retailprice = str_to_double(S[7]);
    copy_string(strings, S[1], 56); // name
    copy_string(strings+57, S[2], 25); // mfgr
    //copy_string(strings+83, S[3], 10); // brand
    copy_string(brandstr, S[3], 10); // brand
    copy_string(strings+83, S[4], 25); // type
    copy_string(strings+109, S[6], 10); // container
    copy_string(strings+120, S[8], 23); // comment
  }
  char* name() {return strings;}
  char* mfgr() {return strings+57;}
  //char* brand() {return strings+83;}
  char* brand() {return brandstr;}
  char* type() {return strings+83;}
  char* container() {return strings+109;}
  char* comment() {return strings+120;}
};


//size 24
struct Part_supp {
  int partkey;
  int suppkey;
  int availqty;
  float supplycost;
  char* cmt;
  Part_supp() {}
  Part_supp(Line const &S) {
    check_length(S, 5);
    partkey = str_to_int(S[0]);
    suppkey = str_to_int(S[1]);
    availqty = str_to_int(S[2]);
    supplycost = str_to_double(S[3]);
    cmt = 0;
  }
};
	
