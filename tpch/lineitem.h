#include "pbbslib/strings/string_basics.h"
#include <string.h>

void copy_string(char* dest, char* src, size_t max_len) {
  if (strlen(src) > max_len) {
    std::cout << "string too long while parsing tables" << std::endl;
    abort();
  }
  strcpy(dest,src);
}

int str_to_int(char* str) {
  return atoi(str);
}

double str_to_double(char* str) {
  return atof(str);
}

using uint = unsigned int;
using ushort = unsigned short;
using uchar = unsigned char;

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

  void from_str(const char* str) {
    char str_cp[11];
    strcpy(str_cp, str);
    str_cp[4]='\0'; str_cp[7] = '\0';
    v.d.year = str_to_int(str_cp) - start_year;
    v.d.month = str_to_int(str_cp+5);
    v.d.day = str_to_int(str_cp+8);
  }

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
  Date(const string x) { from_str(x.c_str());}
  Date(char* str) { from_str(str);}
  
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
  Percent(char* c) : _val((uchar) (atof(c)*100.0)) {}
  double val() {return ((double) _val) * (double) .01;}
  Percent(int p) : _val(p) {}
};

/*8bits. MMMIIRRL
shipmode: M, shipinstruct: I, return flag: F, linestatus: LINESTATUES: L
linestatus(&1): O:1, F:0
return flag (&6): R:0, A:2, N:4
shipinstruct (&24): None(N): 0, TAKE BACK RETURN(T): 8, DELIVER IN PERSON(D):16, COLLECT COD(C): 24
shipmode (&224): RAIL(I): 0, AIR(A): 32, TRUCK(T): 64, FOB(F): 96, REG AIR(G): 128, MAIL(M): 160, SHIP(S): 192
*/
struct lineitem_flags {
  uchar _flags;
  lineitem_flags() {}
  lineitem_flags(int flags) : _flags(flags) {}
  lineitem_flags(char linestatus, char returnflag, char shipinstruct, char shipmode) {
    _flags = (((linestatus == 'O') ? 1 : 0) |
	      ((returnflag == 'R') ? 0 : ((returnflag == 'A') ? 2 : 4)));
	_flags = _flags | (shipinstruct == 'T' ? 8:0) | (shipinstruct == 'D' ? 16:0) | (shipinstruct == 'C' ? 24:0);
	_flags = _flags | (shipmode == 'A' ? 32:0) | (shipmode == 'T' ? 64:0) | (shipmode == 'F' ? 96:0) | (shipmode == 'G' ? 128:0) | (shipmode == 'M' ? 160:0) | (shipmode == 'S' ? 192:0);
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
  Lineitem(sequence<char*> const &S) {
    orderkey = str_to_int(S[0]);
    partkey = str_to_int(S[1]);
    suppkey = str_to_int(S[2]);
    linenumber = str_to_int(S[3]);
    uchar quantity = str_to_int(S[4]);
    e_price = atof(S[5]);
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
block_allocator order_str_pool(96);


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
  Orders() {}
  Orders(sequence<char*> const &S) {
    orderkey = str_to_int(S[0]);
    custkey = str_to_int(S[1]);
    status = S[2][0];
    totalprice = atof(S[3]);
    orderdate = Date(S[4]);
    orderpriority = S[5][0]-48;
    strings = (char*) order_str_pool.alloc();
    for (int i = 0; i < 80; i++) {
		strings[i] = S[8][i];
		if (S[8][i]=='\0') break;
	}
    shippriority = str_to_int(S[7]);
  }
  //char* orderpriority() {return strings;}
  char* clerk() {return strings+80;}
  char* comment() {return strings;}
};

//size 24
struct Customer {
  dkey_t custkey;
  dkey_t nationkey;
  float acctbal;
  char* strings;
  Customer() {}
  Customer(sequence<char*> const &S) {
    custkey = str_to_int(S[0]);
    nationkey = str_to_int(S[3]);
    acctbal = atof(S[5]);
    //cout << "dd" << endl;
    strings = (char*) pbbs::my_alloc(192);
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

//size 24
struct Supplier {
	dkey_t suppkey;
	char* strings;
	float acctbal;
	dkey_t nationkey;
	Supplier() {}
	Supplier(sequence<char*> const &S) {
		suppkey = str_to_int(S[0]);
		nationkey = str_to_int(S[3]);
		acctbal = atof(S[5]);
		strings = (char*) pbbs::my_alloc(185);
		
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
  	Nations(sequence<char*> const &S) {
	  nationkey = str_to_int(S[0]);
	  name = new char[strlen(S[1]+1)];
	  strcpy(name,S[1]);
	  regionkey = str_to_int(S[2]);
	  comment = new char[strlen(S[3]+1)];
	  strcpy(comment,S[3]);
	}
};

sequence<Nations> nations;

block_allocator part_str_pool(155);

//size 24
struct Part {
  dkey_t partkey;
  int size;
  float retailprice;
  char* strings;
  Part() {}
  Part(sequence<char*> const &S) {
    partkey = str_to_int(S[0]);
    strings = (char*) part_str_pool.alloc();
    size = str_to_int(S[5]);
    retailprice = atof(S[7]);
    copy_string(strings, S[1], 56); // name
    copy_string(strings+57, S[2], 25); // mfgr
    copy_string(strings+83, S[3], 10); // brand
    copy_string(strings+94, S[4], 25); // type
    copy_string(strings+120, S[6], 10); // container
    copy_string(strings+131, S[8], 23); // comment
  }
  char* name() {return strings;}
  char* mfgr() {return strings+57;}
  char* brand() {return strings+83;}
  char* type() {return strings+94;}
  char* container() {return strings+120;}
  char* comment() {return strings+131;}
};

//size 24
struct Part_supp {
	int partkey;
	int suppkey;
	int availqty;
	float supplycost;
	char* cmt;
	Part_supp() {}
	Part_supp(sequence<char*> const &S) {
		partkey = str_to_int(S[0]);
		suppkey = str_to_int(S[1]);
		availqty = str_to_int(S[2]);
		supplycost = atof(S[3]);
		cmt = 0;
	}
};
	
