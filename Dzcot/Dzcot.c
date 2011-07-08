/********************************************************************
    created:    2010/02/11 22:12
    file:       Dzcot.c
    author:     Foreverflying
    purpose:    
*********************************************************************/

#include "DzIncOs.h"
#include "Dzcot.h"
#include "DzSynObj.h"
#include "DzCore.h"
#include <assert.h>

// StartHost:
// create the Io mgr co thread, so the host can serve requests
int DzRunHost(
    int         lowestPriority,
    int         defaultPri,
    int         defaultSSize,
    DzRoutine   firstEntry,
    void*       context,
    int         priority,
    int         sSize
    )
{
    assert(
        lowestPriority >= CP_HIGH &&
        lowestPriority < COT_PRIORITY_COUNT
        );
    assert(
        defaultPri >= CP_HIGH &&
        defaultPri <= lowestPriority
        );
    assert(
        defaultSSize >= SS_FIRST &&
        defaultSSize < STACK_SIZE_COUNT
        );
    assert( !GetHost() );
    assert( firstEntry );

    return RunHost( lowestPriority, defaultPri, defaultSSize, firstEntry, context, priority, sSize );
}

// StartCot:
// create a new co thread
int DzStartCot(
    DzRoutine   entry,
    void*       context,
    int         priority,
    int         sSize
    )
{
    DzHost* host = GetHost();
    assert( host );
    assert( entry );
    assert(
        priority == CP_DEFAULT ||
        ( priority >= CP_FIRST && priority <= host->lowestPriority )
        );
    assert(
        sSize >= SS_FIRST &&
        sSize <= STACK_SIZE_COUNT
        );

    return StartCot( host, entry, context, priority, sSize );
}

// StartCot:
// create a new co thread
int DzStartCotInstant(
    DzRoutine   entry,
    void*       context,
    int         priority,
    int         sSize
    )
{
    DzHost* host = GetHost();
    assert( host );
    assert( entry );
    assert(
        priority >= CP_FIRST &&
        priority <= COT_PRIORITY_COUNT
        );
    assert(
        sSize >= SS_FIRST &&
        sSize <= STACK_SIZE_COUNT
        );

    return StartCotInstant( host, entry, context, priority, sSize );
}

int DzGetCotCount()
{
    DzHost* host = GetHost();
    assert( host );

    return GetCotCount( host );
}

int DzGetMaxCotCount( BOOL reset )
{
    DzHost* host = GetHost();
    assert( host );

    return GetMaxCotCount( host, reset );
}

int DzChangePriority( int priority )
{
    DzHost* host = GetHost();
    assert( host );
    assert( priority > CP_FIRST && priority <= host->lowestPriority );

    return SetCurrCotPriority( host, priority );
}

BOOL DzGrowCotPoolDepth( int sSize, int deta )
{
    DzHost* host = GetHost();
    assert(
        sSize > DZ_MAX_PERSIST_STACK_SIZE &&
        sSize <= STACK_SIZE_COUNT
        );

    return GrowCotPoolDepth( host, sSize, deta );
}

int DzWaitSynObj( DzHandle obj, int timeout )
{
    DzHost* host = GetHost();
    assert( host );
    assert( obj );

    return WaitSynObj( host, obj, timeout );
}

int DzWaitMultiSynObj( int count, DzHandle* obj, BOOL waitAll, int timeout )
{
    DzHost* host = GetHost();
    assert( host );
    assert( obj );
    assert( count > 0 );

    return WaitMultiSynObj( host, count, obj, waitAll, timeout );
}

DzHandle DzCreateMtx( BOOL owner )
{
    DzHost* host = GetHost();
    assert( host );

    return CreateEvt( host, FALSE, !owner );
}

BOOL DzReleaseMtx( DzHandle mtx )
{
    DzHost* host = GetHost();
    assert( host );
    assert( mtx );
    assert( mtx->type == TYPE_EVT_AUTO );

    SetEvt( host, mtx );
    return TRUE;
}

DzHandle DzCreateEvt( BOOL manualReset, BOOL notified )
{
    DzHost* host = GetHost();
    assert( host );

    return CreateEvt( host, manualReset, notified );
}

BOOL DzSetEvt( DzHandle evt )
{
    DzHost* host = GetHost();
    assert( host );
    assert( evt );
    assert( evt->type == TYPE_EVT_AUTO || evt->type == TYPE_EVT_MANUAL );

    SetEvt( host, evt );
    return TRUE;
}

BOOL DzResetEvt( DzHandle evt )
{
    assert( GetHost() );
    assert( evt );
    assert( evt->type == TYPE_EVT_AUTO || evt->type == TYPE_EVT_MANUAL );

    ResetEvt( evt );
    return TRUE;
}

DzHandle DzCreateSem( int count )
{
    DzHost* host = GetHost();
    assert( host );
    assert( count >= 0 );

    return CreateSem( host, count );
}

int DzReleaseSem( DzHandle sem, int count )
{
    DzHost* host = GetHost();
    assert( host );
    assert( sem );
    assert( sem->type == TYPE_SEM );
    assert( count > 0 );
    assert( sem->count + count > sem->count );

    return ReleaseSem( host, sem, count );
}

DzHandle DzCloneSynObj( DzHandle obj )
{
    assert( GetHost() );
    assert( obj );
    assert( obj->type != TYPE_CALLBACK_TIMER );

    return CloneSynObj( obj );
}

BOOL DzCloseSynObj( DzHandle obj )
{
    DzHost* host = GetHost();
    assert( host );
    assert( obj );
    assert( obj->type != TYPE_TIMER && obj->type != TYPE_CALLBACK_TIMER );

    CloseSynObj( host, obj );
    return TRUE;
}

DzHandle DzCreateTimer( uint milSec, uint repeat )
{
    DzHost* host = GetHost();
    assert( host );
    assert( (int)milSec > 0 );
    assert( repeat < 65536 );

    return CreateTimer( host, (int)milSec, (unsigned short)repeat );
}

BOOL DzCloseTimer( DzHandle timer )
{
    DzHost* host = GetHost();
    assert( host );
    assert( timer );
    assert( timer->type = TYPE_TIMER );

    CloseTimer( host, timer );
    return TRUE;
}

DzHandle DzCreateCallbackTimer(
    uint            milSec,
    uint            repeat,
    DzRoutine       callback,
    void*           context,
    int             priority,
    int             sSize
    )
{
    DzHost* host = GetHost();
    assert( host );
    assert( (int)milSec > 0 );
    assert( callback );
    assert(
        priority == CP_DEFAULT ||
        ( priority >= CP_FIRST && priority <= host->lowestPriority )
        );
    assert(
        sSize >= SS_FIRST &&
        sSize <= STACK_SIZE_COUNT
        );
    assert( repeat < 65536 );

    return CreateCallbackTimer( host, (int)milSec, (unsigned short)repeat, callback, context, priority, sSize );
}

BOOL DzCloseCallbackTimer( DzHandle timer )
{
    DzHost* host = GetHost();
    assert( host );
    assert( timer );
    assert( timer->type = TYPE_CALLBACK_TIMER );

    CloseCallbackTimer( host, timer, TRUE );
    return TRUE;
}

int DzSleep( uint milSec )
{
    DzHost* host = GetHost();
    assert( host );
    assert( (int)milSec > 0 );

    DelayCurrThread( host, (int)milSec );
    return DS_OK;
}

int DzSleep0()
{
    DzHost* host = GetHost();
    assert( host );

    DispatchCurrThread( host );
    return DS_OK;
}

int DzOpenFileA( const char* fileName, int flags )
{
    DzHost* host = GetHost();
    assert( host );

    return OpenA( host, fileName, flags );
}

int DzOpenFileW( const wchar_t* fileName, int flags )
{
    DzHost* host = GetHost();
    assert( host );

    return OpenW( host, fileName, flags );
}

int DzCloseFd( int fd )
{
    DzHost* host = GetHost();
    assert( host );

    return Close( host, fd );
}

size_t DzReadFile( int fd, void* buf, size_t count )
{
    DzHost* host = GetHost();
    assert( host );

    return Read( host, fd, buf, count );
}

size_t DzWriteFile( int fd, const void* buf, size_t count )
{
    DzHost* host = GetHost();
    assert( host );

    return Write( host, fd, buf, count );
}

size_t DzSeekFile( int fd, size_t offset, int whence )
{
    DzHost* host = GetHost();
    assert( host );

    return Seek( host, fd, offset, whence );
}

size_t DzGetFileSize( int fd )
{
    DzHost* host = GetHost();

    return FileSize( host, fd );
}

BOOL DzSockStartup()
{
    return SockStartup();
}

BOOL DzSockCleanup()
{
    return SockCleanup();
}

int DzSocket( int domain, int type, int protocol )
{
    DzHost* host = GetHost();
    assert( host );
    assert( isSocketStarted );

    return Socket( host, domain, type, protocol );
}

int DzShutdown( int fd, int how )
{
    assert( GetHost() );
    assert( isSocketStarted );

    return Shutdown( fd, how );
}

int DzCloseSocket( int fd )
{
    DzHost* host = GetHost();
    assert( host );
    assert( isSocketStarted );

    return CloseSocket( host, fd );
}

int DzBind( int fd, struct sockaddr* addr, int addrLen )
{
    assert( GetHost() );
    assert( isSocketStarted );

    return Bind( fd, addr, addrLen );
}

int DzListen( int fd, int backlog )
{
    assert( GetHost() );
    assert( isSocketStarted );

    return Listen( fd, backlog );
}

int DzConnect( int fd, struct sockaddr* addr, int addrLen )
{
    DzHost* host = GetHost();
    assert( host );
    assert( isSocketStarted );

    return Connect( host, fd, addr, addrLen );
}

int DzAccept( int fd, struct sockaddr* addr, int* addrLen )
{
    DzHost* host = GetHost();
    assert( host );
    assert( isSocketStarted );

    return Accept( host, fd, addr, addrLen );
}

int DzSendEx( int fd, DzBuf* bufs, int bufCount, int flags )
{
    DzHost* host = GetHost();
    assert( host );
    assert( isSocketStarted );

    return SendEx( host, fd, bufs, bufCount, flags );
}

int DzRecvEx( int fd, DzBuf* bufs, int bufCount, int flags )
{
    DzHost* host = GetHost();
    assert( host );
    assert( isSocketStarted );

    return RecvEx( host, fd, bufs, bufCount, flags );
}

int DzSendToEx( int fd, DzBuf* bufs, int bufCount, int flags, const struct sockaddr *to, int tolen )
{
    DzHost* host = GetHost();
    assert( host );
    assert( isSocketStarted );

    return SendToEx( host, fd, bufs, bufCount, flags, to, tolen );
}

int DzRecvFromEx( int fd, DzBuf* bufs, int bufCount, int flags, struct sockaddr *from, int *fromlen )
{
    DzHost* host = GetHost();
    assert( host );
    assert( isSocketStarted );

    return RecvFromEx( host, fd, bufs, bufCount, flags, from, fromlen );
}

int DzSend( int fd, const void* buf, int len, int flags )
{
    DzHost* host = GetHost();
    assert( host );
    assert( isSocketStarted );

    return Send( host, fd, buf, len, flags );
}

int DzRecv( int fd, void* buf, int len, int flags )
{
    DzHost* host = GetHost();
    assert( host );
    assert( isSocketStarted );

    return Recv( host, fd, buf, len, flags );
}

int DzSendTo( int fd, const char *buf, int len, int flags, const struct sockaddr *to, int tolen )
{
    DzHost* host = GetHost();
    assert( host );
    assert( isSocketStarted );

    return SendTo( host, fd, buf, len, flags, to, tolen );
}

int DzRecvFrom( int fd, char *buf, int len, int flags, struct sockaddr *from, int *fromlen )
{
    DzHost* host = GetHost();
    assert( host );
    assert( isSocketStarted );

    return RecvFrom( host, fd, buf, len, flags, from, fromlen );
}

DzParamNode* DzAllocParamNode()
{
    DzHost* host = GetHost();
    assert( host );

    return (DzParamNode*)AllocQNode( host );
}

void DzFreeParamNode( DzParamNode* node )
{
    DzHost* host = GetHost();
    assert( host );
    assert( node );

    FreeQNode( host, (DzLNode*)node );
}

void* DzMalloc( size_t size )
{
    DzHost* host = GetHost();
    assert( host );

    return Malloc( host, size );
}

void* DzCalloc( size_t num, size_t size )
{
    DzHost* host = GetHost();
    assert( host );

    return Calloc( host, num, size );
}

void* DzReAlloc( void* mem, size_t size )
{
    DzHost* host = GetHost();
    assert( host );

    return ReAlloc( host, mem, size );
}

void DzFree( void* mem )
{
    DzHost* host = GetHost();
    assert( host );
    assert( mem );

    Free( host, mem );
}

unsigned long long DzUnixTime()
{
    return (unsigned long long)UnixTime();
}

unsigned long long DzMilUnixTime()
{
    return (unsigned long long)MilUnixTime();
}
