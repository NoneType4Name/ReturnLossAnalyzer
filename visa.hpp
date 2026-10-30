#pragma once
#include <cstdint>
#include <vector>
#include <visa.h>
#include <string>
#include <stdexcept>
#include <cstring>
#include <visatype.h>

#define STR_IMPL( x ) #x
#define STR( x )      STR_IMPL( x )

// Магия компилятора: __builtin_strrchr находит последний слэш на этапе компиляции,
// чтобы в исключении оставалось только "visa.hpp:131", а не "C:\Users\...\visa.hpp:131"
#define AT_LINE_FILENAME ( __builtin_strrchr( __FILE__, '\\' ) ? __builtin_strrchr( __FILE__, '\\' ) + 1 : ( __builtin_strrchr( __FILE__, '/' ) ? __builtin_strrchr( __FILE__, '/' ) + 1 : __FILE__ ) )

#define AT_LINE ( std::string( AT_LINE_FILENAME ) + ":" + STR( __LINE__ ) )

class VisaError : public std::runtime_error
{
  public:
    VisaError( ViStatus st, const std::string &ctx, const std::string &msg ) :
        std::runtime_error( msg ), status_( st ), context_( ctx ) {}
    ViStatus status() const
    {
        return status_;
    }
    const std::string &context() const
    {
        return context_;
    }

  private:
    ViStatus status_;
    std::string context_;
};

struct ScpiError
{
    int code = 0;
    std::string message;
    bool ok() const
    {
        return code == 0;
    }
    std::string str() const
    {
        return "[" + std::to_string( code ) + "] " + message;
    }
};

class VisaInstrument
{
  public:
    explicit VisaInstrument( ViSession inst,
                             uint32_t timeout_ms    = 1000,
                             bool check_after_write = false ) :
        instr_( inst ), check_after_write_( check_after_write )
    {
        viSetAttribute( instr_, VI_ATTR_TMO_VALUE, timeout_ms );
        viSetAttribute( instr_, VI_ATTR_TERMCHAR, '\n' );
        viSetAttribute( instr_, VI_ATTR_TERMCHAR_EN, VI_TRUE );

        viClear( instr_ );
        viFlush( instr_, VI_WRITE_BUF | VI_READ_BUF );
    }

    VisaInstrument( const VisaInstrument & )            = delete;
    VisaInstrument &operator=( const VisaInstrument & ) = delete;

    VisaInstrument( VisaInstrument &&o ) noexcept
        :
        instr_( o.instr_ ), addr_( std::move( o.addr_ ) ), check_after_write_( o.check_after_write_ )
    {
        o.instr_ = 0;
    }
    VisaInstrument &operator=( VisaInstrument &&o ) noexcept
    {
        if ( this != &o )
        {
            close();
            instr_             = o.instr_;
            addr_              = std::move( o.addr_ );
            check_after_write_ = o.check_after_write_;
            o.instr_           = 0;
        }
        return *this;
    }

    ~VisaInstrument()
    {
        close();
    }

    void write( const std::string &cmd )
    {
        rawWrite( cmd );
        if ( check_after_write_ )
        {
            ScpiError e = lastError();
            if ( !e.ok() )
            {
                throw VisaError( -1, "write(" + cmd + ")", e.str() );
            }
        }
    }

    void writeNoCheck( const std::string &cmd )
    {
        rawWrite( cmd );
    }

    std::string query( const std::string &cmd )
    {
        rawWrite( cmd );
        std::string r = rawRead();
        if ( check_after_write_ )
        {
            ScpiError e = lastError();
            if ( !e.ok() )
                throw VisaError( -1, "query(" + cmd + ")", e.str() );
        }
        return r;
    }

    void rawWrite( const std::string &cmd )
    {
        std::string s = cmd;
        if ( s.empty() || s.back() != '\n' ) s += '\n';

        ViUInt32 written = 0;
        ViStatus st      = viWrite( instr_,
                                    reinterpret_cast<ViBuf>( const_cast<char *>( s.data() ) ),
                                    static_cast<ViUInt32>( s.size() ),
                                    &written );
        if ( st < VI_SUCCESS )
            throw VisaError( st, "viWrite", AT_LINE );
        if ( written != s.size() )
            throw VisaError( -1, "viWrite", "Partial write" );
    }

    std::string rawRead()
    {
        std::string out;
        char buf[ 4096 ];
        ViUInt32 got = 0;
        for ( ;; )
        {
            ViStatus st = viRead( instr_,
                                  reinterpret_cast<ViBuf>( buf ),
                                  sizeof( buf ),
                                  &got );
            if ( got > 0 ) out.append( buf, got );

            if ( st == VI_SUCCESS_TERM_CHAR || st == VI_SUCCESS ) break;
            else if ( st < 0 )
                throw VisaError( st, "viRead", AT_LINE );
        }
        return out;
    }

    std::vector<double> rawDataRead()
    {
        uint32_t tmo;
        char buf[ 9 ];
        ViUInt32 got   = 0;
        ViStatus st    = VI_SUCCESS;
        char *hash_ptr = nullptr;
        viGetAttribute( instr_, VI_ATTR_TMO_VALUE, &tmo );
        viSetAttribute( instr_, VI_ATTR_TERMCHAR_EN, VI_FALSE );
        viSetAttribute( instr_, VI_ATTR_TMO_VALUE, 100 );

        while ( !hash_ptr )
        {
            st = viRead( instr_, reinterpret_cast<ViBuf>( buf ), 1, &got );
            if ( st < 0 ) throw VisaError( st, "viRead", AT_LINE );
            if ( got == 0 ) throw std::runtime_error( "No data received" );
            if ( buf[ 0 ] == '#' )
                hash_ptr = buf;
        }

        st = viRead( instr_, reinterpret_cast<ViBuf>( buf ), 1, &got );
        if ( st < 0 ) throw VisaError( st, "viRead", AT_LINE );
        if ( got == 0 ) throw std::runtime_error( "No data received" );
        hash_ptr = buf;

        int num_digits = *( hash_ptr ) - '0';
        if ( num_digits < 1 || num_digits > 9 ) throw std::runtime_error( "Invalid header format" );

        st = viRead( instr_, reinterpret_cast<ViBuf>( buf ), num_digits, &got );
        if ( st < 0 ) throw VisaError( st, "viRead", AT_LINE );
        if ( got == 0 ) throw std::runtime_error( "No data received" );

        size_t total_bytes = std::stoul( buf );
        std::vector<double> out_data( total_bytes / sizeof( double ) );

        st = viRead( instr_, reinterpret_cast<ViBuf>( out_data.data() ), total_bytes, &got );
        if ( st < 0 && st != VI_ERROR_TMO ) throw VisaError( st, "viRead", AT_LINE );
        if ( got == 0 ) throw std::runtime_error( "No data received" );

        viSetAttribute( instr_, VI_ATTR_TERMCHAR_EN, VI_TRUE );
        viSetAttribute( instr_, VI_ATTR_TMO_VALUE, tmo );
        return out_data;
    }

    ScpiError lastError()
    {
        ScpiError first;
        for ( int i = 0; i < 5; ++i )
        {
            rawWrite( "SYST:ERR?" );
            std::string r = rawRead();

            ScpiError e = parseScpiError( r );
            if ( i == 0 ) first = e;
            if ( e.ok() ) break;
        }
        return first;
    }

    void waitComplete()
    {
        query( "*OPC?" );
    }

    // bool tryWaitComplete( unsigned poll_ms = 50, unsigned max_ms = 10000 )
    // {
    //     auto t0 = std::chrono::steady_clock::now();
    //     while ( std::chrono::duration_cast<std::chrono::milliseconds>(
    //                 std::chrono::steady_clock::now() - t0 )
    //                 .count() < max_ms )
    //     {
    //         try
    //         {
    //             std::string r = query( "*OPC?" );
    //             if ( !r.empty() && r[ 0 ] == '1' ) return true;
    //         }
    //         catch ( ... )
    //         {
    //         }
    //         std::this_thread::sleep_for(
    //             std::chrono::milliseconds( poll_ms ) );
    //     }
    //     return false;
    // }

    ViSession session() const
    {
        return instr_;
    }
    const std::string &address() const
    {
        return addr_;
    }

  private:
    static ScpiError parseScpiError( const std::string &s )
    {
        ScpiError e;
        auto comma = s.find( ',' );
        if ( comma == std::string::npos )
        {
            if ( !s.empty() && ( s[ 0 ] == '0' || s[ 0 ] == '\n' ) ) return e;
            e.code    = -1;
            e.message = s;
            return e;
        }
        try
        {
            e.code = std::stoi( s.substr( 0, comma ) );
        }
        catch ( ... )
        {
            e.code = -1;
        }

        std::string msg = s.substr( comma + 1 );

        if ( msg.size() >= 2 && msg.front() == '"' && msg.back() == '"' )
            msg = msg.substr( 1, msg.size() - 2 );
        e.message = msg;
        return e;
    }

    void close() noexcept
    {
        if ( instr_ )
        {
            viClose( instr_ );
            instr_ = 0;
        }
    }

    ViSession instr_ = 0;
    std::string addr_;
    bool check_after_write_ = true;
};