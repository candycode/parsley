#ifndef INSTREAM_H_
#define INSTREAM_H_

/// @file InStream.h Implementation of input stream wrapper; required to perform
///       additional operations such as keeping the number of read lines.

#include <string>
#include <cassert>
#include <algorithm>
#include <stdexcept>
#include <deque>
#include <functional>
#include <stdexcept>
#include <locale>

//this required if InChStream is used with std::istream and 
//included before iostream to properly resolve typedef typename IT::* types (on gcc).
//#include <iostream>


/// @brief Input character stream class featuring automatic tracking of current line and char.
///
/// All method calls are forwarded to the equivalent methods in the contained input stream
/// instance.
/// Only @c get @c unget @c peek @c tellg and @c seekg are implemented.
/// @tparam IT input stream e.g. @c std::istream
/// @tparam RESIZE_THRESHOLD max number of end of line separators to keep in memory
/// @tparam EOL_ end of line character
/// @ingroup utility
template < class IT,
           unsigned long RESIZE_THRESHOLD = 0xffffff,
           typename IT::char_type EOL_ = '\n' >
class InChStream
{
public:
    ///Constructor.
    ///@param is input stream.
    InChStream( IT& is ) :  isp_( &is ), lines_( 0 ),
                            lineChars_( 0 ), eolIt_( eols_.begin() ),
                            locale_( is.getloc() )
    {
#ifndef BUFFERED_IN_STREAM
        //@warning required on MS Windows with VC++, issues in istreams require the stream to be
        //NOT buffered for tellg/unget/putback/seekg to work properly. Doesn't harm on other platforms.
        assert( is.rdbuf() && "NULL rdbuf" );   
        is.rdbuf()->pubsetbuf( 0, 0 );
#endif
    }
    typedef typename IT::char_type char_type;
    typedef typename IT::streampos streampos;
    typedef typename IT::streamoff streamoff;
    typedef std::deque< streamoff > EOLs;
    /// Returns current char and moves get pointer to next character.
    /// @return current char in stream buffer.
    /// @throw std::logic_error if stream @c good bit @b not set.
    char_type get() 
    {
        assert( isp_ != 0 && "NULL STREAM POINTER" );
        char_type c = 0;
        if( isp_->good() ) {
            c = isp_->get();
        } else {
            throw std::logic_error( "Attempt to read from invalid stream" );
            return c; // in case exception handling disabled
        } 
        if( c == EOL_ )
        {
            ++lines_;
            lineChars_ = 0;
            eols_.push_back( streamoff( isp_->tellg() ) - 1 );
        }
        else ++lineChars_;
        // this is to ensure that the eol list doesn't get too big, could
        // use a custom fixed size circular buffer
        if( eols_.size() > RESIZE_THRESHOLD )
        {
            eols_.erase( eols_.begin(), eols_.begin() + RESIZE_THRESHOLD / 2 );
        }
        return c;
    }
    
    /// Returns current character in stream without moving the get pointer.
    /// @return current character.
    /// @throw @c std::logic_error if @c good bit not set.
    char_type peek() 
    {
        assert( isp_ != 0 && "NULL STREAM POINTER" );
        char_type c = 0;
        if( isp_->good() ) c = isp_->peek();
        else
        {
            throw std::logic_error( "Attempt to read from invalid stream" );
            return c; // in case exception handling disabled
        }       
        return c;
    }

    /// Clears bits.
    void clear() 
    {
        assert( isp_ != 0 && "NULL STREAM POINTER" );
        isp_->clear();
    }

    /// Returns current get pointer position.
    /// @return get pointer position.
    streampos tellg()
    {
        assert( isp_ != 0 && "NULL STREAM POINTER" );
        return isp_->tellg();
    }

    /// Moves get pointer to the specified position. In case of forward movement
    /// it repeatedly invokes Any::get() and records end of line separators.
    /// @param p stream position
    /// @return reference to stream
    IT& seekg( streampos p )
    {
        assert( isp_ != 0 && "NULL STREAM POINTER" );
        if( isp_->tellg() == p ) return *isp_;
        if( !isp_->good() ) clear();
        
        if( isp_->tellg() > p ) BackwardSeek( p );
        else ForwardSeek( p );
        
        return *isp_;
    }

    /// Move back get pointer.
    void unget()
    {
        assert( isp_ != 0 && "NULL STREAM POINTER" );
        if( streamoff( isp_->tellg() ) == 0  ) return;
        clear(); //make unget work after eof is reached
        isp_->unget();
        const char_type  PEEKED = isp_->peek();
        if( PEEKED == EOL_ )
        {
            --lines_;
            eolIt_ = eols_.end(); --eolIt_;
            eols_.erase( eolIt_, eols_.end() );
        }
        if( !eols_.empty() ) lineChars_ = int( isp_->tellg() - eols_.back() ); 
    }

    /// Returns @c good bit.
    /// @return @c good bit.
    bool good() const 
    { 
        assert( isp_ != 0 && "NULL STREAM POINTER" );
        return isp_->good(); 
    }

    /// Returns @c eof bit.
    /// @return @c eof bit.
    bool eof() 
    { 
        assert( isp_ != 0 && "NULL STREAM POINTER" );
        if( isp_->good() ) return false;
        if( isp_->eof() ) return true;
        unget();
        return get() != -1; 
    }
    
    /// Returns number of lines read.
    /// @return number of lines.
    int get_lines() const { return lines_; }
    
    /// Returns number of characters read in last line.
    /// @return character read in last line.
    int get_line_chars() const { return lineChars_; }

    /// Returns locale.
    const std::locale& getloc() const 
    { 
        return locale_;
    }

private:
    /// Seek backward. Called by Any::seekg.
    void BackwardSeek( streampos p )
    {
        assert( isp_ != 0 && "NULL STREAM POINTER" );
        isp_->seekg( p );
        const streamoff sp = p; 
        reolIt_ = std::find_if( eols_.rbegin(), eols_.rend(),
                    std::bind1st( std::greater< streamoff >(), sp ) );
        if( reolIt_ == eols_.rend() ) return;
        eolIt_ = reolIt_.base(); //reverse to forward iterator
        eols_.erase( eolIt_, eols_.end() );
        lines_ = eols_.size();
        if( !eols_.empty() ) lineChars_ = int( p - eols_.back() ); 
    }
    /// Seek forward. Called by Any::seekg.
    void ForwardSeek( streampos p )
    {
        assert( isp_ != 0 && "NULL STREAM POINTER" );
        while( !isp_->bad() && isp_->tellg() != p ) get();
    }
    /// Pointer to input stream. Life cycle is handled outside this class.
    IT* isp_;
    /// Number of line separator read.
    int lines_;
    /// Number of characters read in current line.
    int lineChars_;
    /// Positions of read end of line separators. Used to track the number
    /// of lines read. The maximum size of the sequence is @c RESIZE_THRESHOLD.
    /// If resizing is needed then the first @c RESIZE_THRESHOLD characters are
    /// removed.
    EOLs eols_;
    /// Convenience variable to store temporary iterarator in end of line list. 
    typename EOLs::iterator eolIt_;
    /// Convenience variable to store temporary reverse iterator in end of line list. 
    typename EOLs::reverse_iterator reolIt_;
    /// Current locale, recorded at construction time.
    const std::locale locale_;
};

/// Convenience typedef for stadard streams.
/// @ingroup utility
typedef InChStream< std::istream > InCharStream;

#endif //INSTREAM_H_

