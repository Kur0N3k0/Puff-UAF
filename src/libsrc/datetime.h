// obtained from  http://sourceforge.net/snippet/  on April 16, 2003
// 
// datetime.h - date and time class declarations
////////////////////////////////////////////////////////////////////////

#ifndef _datetime_h_
#define _datetime_h_

#include <string>

using namespace std;

namespace datetime {

    class Error { };
    class Invalid : public Error { };

    inline string integer_to_string(int I, string::size_type min_length = 1) throw()
    {
        string s;
        while(I) {
            s = char((I % 10) + '0') + s;
            I /= 10;
        }
        while(s.length() < min_length) {
            s = '0' + s;
        }
        return s;
    }

    bool is_leap_year(int) throw();
    int days_in_year(int) throw();
    int days_in_month(int, int) throw(Error);
    bool is_valid_date(int, int, int) throw();
    string day_name(int, int, int) throw(Error);
    string month_name(int) throw(Error);
    string year_name(int) throw();

    class date {

    private:
    
        int dy, dm, dd;
        void bound() throw();

    public:

        date() throw() : dy(1), dm(1), dd(1) { }
        date(int y, int m, int d) throw(Invalid) : dy(y), dm(m), dd(d) { if(!is_valid_date(y, m, d)) throw Invalid(); }
        date(const date& d) throw() : dy(d.dy), dm(d.dm), dd(d.dd) { }
        
        int day() const throw() { return dd; }
        date& day(int I) throw(Invalid) { if(!is_valid_date(dy, dm, I)) throw Invalid(); dd = I; return *this; }
        int month() const throw() { return dm; }
        date& month(int I) throw(Invalid) { if(!is_valid_date(dy, I, dd)) throw Invalid(); dm = I; return *this; }
        int year() const throw() { return dy; }
        date& year(int I) throw(Invalid) { if(!is_valid_date(I, dm, dd)) throw Invalid(); dy = I; return *this; }
        
        bool operator==(const date& d) const throw() { return (dy == d.dy && dm == d.dm && dd == d.dd); }
        bool operator!=(const date& d) const throw() { return !operator==(d); }
        bool operator< (const date& d) const throw() { return (dy < d.dy || dm < d.dm || dd < d.dd); }
        bool operator<=(const date& d) const throw() { return (operator==(d) || operator<(d)); }
        bool operator> (const date& d) const throw() { return !operator<=(d); }
        bool operator>=(const date& d) const throw() { return !operator<(d); }

        date& operator++() throw() { dd++; bound(); return *this; }
        date operator++(int I) throw() { (void)I; date w(*this); dd++; bound(); return w; }
        date& operator--() throw() { dd--; bound(); return *this; }
        date operator--(int I) throw() { (void)I; date w(*this); dd--; bound(); return w; }
        
        date& operator+=(int I) throw() { dd += I; bound(); return *this; }
        date& operator-=(int I) throw() { dd -= I; bound(); return *this; }
        date& operator=(const date& d) throw() { dd = d.dd; dm = d.dm; dy = d.dy; return *this; }

        string format_name(const string&) const throw();
        operator string() const throw();
        string day_name() const throw();
        string month_name() const throw();
        string year_name() const throw();

    };

    date operator+(int, const date&) throw();
    date operator+(const date&, int) throw();
    date operator-(const date&, int) throw();

    int operator-(const date&, const date&) throw();
    int difference(const date&, const date&) throw();

    date get_current_date() throw();




    bool is_valid_time(int, int, int) throw();

    class time {
        
    private:
    
        int th, tm, ts;
        void bound() throw();

        int gmt_offset;

    public:
    
        time() throw() : th(0), tm(0), ts(0), gmt_offset(0) { }
        time(int H, int M, int S, int G = 0) throw(Invalid) : th(H), tm(M), ts(S), gmt_offset(G) { if(!is_valid_time(H, M, S)) throw Invalid(); }
        time(int s) throw() : th(0), tm(0), ts(s), gmt_offset(0) { bound(); }
        time(const time& t) throw() : th(t.th), tm(t.tm), ts(t.ts), gmt_offset(t.gmt_offset) { }
        
        int hour() const throw() { return th; }
        time& hour(int I) throw(Invalid) { if(!is_valid_time(I, tm, ts)) throw Invalid(); th = I; return *this; }
        int minute() const throw() { return tm; }
        time& minute(int I) throw(Invalid) { if(!is_valid_time(th, I, ts)) throw Invalid(); tm = I; return *this; }
        int second() const throw() { return ts; }
        time& second(int I) throw(Invalid) { if(!is_valid_time(th, tm, I)) throw Invalid(); ts = I; return *this; }

        int gmt_diff() const throw() { return gmt_offset; }
        time& gmt_diff(int I) throw() { gmt_offset = I; return *this; }
        time& change_timezone_to(int I) throw() { ts -= gmt_offset - I; bound(); gmt_offset = I; return *this; }
        time& change_timezone_by(int I) throw() { ts -= I; gmt_offset -= I; bound(); return *this; }

        bool operator==(const time& t) const throw() { return (time_in_seconds() == t.time_in_seconds()); }
        bool operator!=(const time& t) const throw() { return !operator==(t); }
        bool operator< (const time& t) const throw() { return (time_in_seconds() == t.time_in_seconds()); }
        bool operator<=(const time& t) const throw() { return (operator==(t) || operator<=(t)); }
        bool operator> (const time& t) const throw() { return !operator<=(t); }
        bool operator>=(const time& t) const throw() { return !operator<(t); }

        time& operator++() throw() { ts++; bound(); return *this; }
        time operator++(int I) throw() { (void)I; time w(*this); ts++; bound(); return w; }
        time& operator--() throw() { ts--; bound(); return *this; }
        time operator--(int I) throw() { (void)I; time w(*this); ts--; bound(); return w; }

        time& operator+=(int I) throw() { ts += I; bound(); return *this; }
        time& operator+=(const time& t) throw() { ts += t.time_in_seconds(); bound(); return *this; }
        time& operator-=(int I) throw() { ts -= I; bound(); return *this; }
        time& operator-=(const time& t) throw() { ts -= t.time_in_seconds(); bound(); return *this; }
        time& operator=(const time& t) throw() { th = t.th; tm = t.tm; ts = t.ts; gmt_offset = t.gmt_offset; return *this; }

        string format_name(const string&) const throw();
        operator string() const throw();
        int time_in_seconds() const throw() { return (ts + tm * 60 + th * 3600) + gmt_offset; }
        operator int() const throw() { return time_in_seconds(); }
    };

    time operator+(const time&, const time&) throw();
    time operator+(const time&, int) throw();
    time operator+(int, const time&) throw();
    time operator-(const time&, const time&) throw();
    time operator-(const time&, int) throw();

    time difference(const time&, const time&) throw();
    
    time get_current_time() throw();
    time seconds_to_time(int) throw();
}

#endif // _datetime_h_



