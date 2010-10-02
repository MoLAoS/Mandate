#ifndef SIMPLESYMBOLENGINE_H
#define SIMPLESYMBOLENGINE_H

#include <iosfwd>
#include <string>
#include <windows.h>
#include <dbghelp.h>

/** Symbol Engine wrapper to assist with processing PDB information */
class DebugSymbolEngine
{
public:
    /* Get the symbol engine for this process */
    static DebugSymbolEngine &instance();

    /** Convert an address to a string */
    std::string addressToString( PVOID address );

    /** Provide a stack trace for the specified stack frame */
    void StackTrace( PCONTEXT pContext, std::ostream & os );

private:
    /* don't copy or assign */
    DebugSymbolEngine( DebugSymbolEngine const & );
    DebugSymbolEngine& operator=( DebugSymbolEngine const & );

    /* Construct wrapper for this process */
    DebugSymbolEngine();

public: // Work around for MSVC 6 bug
    /* Destroy information for this process */
    ~DebugSymbolEngine();
private:
};

#endif // SIMPLESYMBOLENGINE_H
