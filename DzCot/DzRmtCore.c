/********************************************************************
    created:    2011/10/13 14:42
    file:       DzRmtCore.c
    author:     Foreverflying
    purpose:    
********************************************************************/

#include "DzRmtCore.h"
#include "DzCore.h"

void __stdcall PauseCotHelpEntry( intptr_t context )
{
    DzHost* host = GetHost();
    DzCot* cot = host->currCot;
    DzCot* dzCot = (DzCot*)context;

    if( cot->feedType ){
        SendRmtCot( host, cot->hostId, cot->feedType < 0, dzCot );
    }else{
        AddLItrToTail( &host->lazyRmtCot[ cot->hostId ], &dzCot->lItr );
        if( !host->lazyTimer ){
            StartLazyTimer( host );
        }
    }
}

inline void MoveCurCotToRmt( DzHost* host, int rmtId, int feedType )
{
    DzCot* helpCot;

    helpCot = AllocDzCot( host, SS_FIRST );
    SetCotEntry( helpCot, PauseCotHelpEntry, (intptr_t)host->currCot );
    helpCot->hostId = rmtId;
    helpCot->feedType = feedType;
    SwitchToCot( host, helpCot );
}

void __stdcall RemoteCotEntry( intptr_t context )
{
    DzHost* host = GetHost();
    DzCot* dzCot = host->currCot;

    dzCot->entry( context );
    MoveCurCotToRmt( host, dzCot->hostId, dzCot->feedType );
    host = GetHost();
    host->scheduleCd++;
    if( dzCot->feedType > 0 ){
        if( dzCot->evtType == 0 ){
            SetEvt( host, dzCot->evt );
            CloseSynObj( host, dzCot->evt );
        }else{
            NotifyEasyEvt( host, dzCot->easyEvt );
        }
    }
}

void __stdcall LazyFreeMemEntry( intptr_t context )
{
    int rmtId;
    DzLNode* node;
    DzLItr* tail;
    DzHost* host;

    host = GetHost();
    host->scheduleCd++;
    node = MEMBER_BASE( (DzLItr*)context, DzLNode, lItr );
    tail = (DzLItr*)node->d2;
    rmtId = (int)node->d3;
    do{
        Free( host, (void*)node->d1 );
        node = MEMBER_BASE( node->lItr.next, DzLNode, lItr );
    }while( node );
    MoveCurCotToRmt( host, rmtId, 1 );
    host = GetHost();
    host->scheduleCd++;
    FreeChainLNode( host, (DzLItr*)context, tail );
}

void __stdcall DealLazyResEntry( intptr_t context )
{
    int i;
    DzLItr* lItr;
    DzLItr* tail;
    DzCot* dzCot;
    DzHost* host;
    DzLNode* node;

    host = GetHost();
    for( i = 0; i < host->hostCount; i++ ){
        if( !IsSListEmpty( host->lazyRmtCot + i ) ){
            tail = host->lazyRmtCot[i].tail;
            lItr = GetChainAndResetSList( host->lazyRmtCot + i );
            dzCot = MEMBER_BASE( lItr, DzCot, lItr );
            dzCot->priority -= CP_DEFAULT + 1;
            if( SendRmtCot( host, i, FALSE, dzCot ) == FALSE ){
                dzCot->priority += CP_DEFAULT + 1;
                host->pendRmtCot[i].tail = tail;
            }
        }
        if( !IsSListEmpty( host->lazyFreeMem + i ) ){
            tail = host->lazyFreeMem[i].tail;
            lItr = GetChainAndResetSList( host->lazyFreeMem + i );
            node = MEMBER_BASE( lItr, DzLNode, lItr );
            node->d2 = (intptr_t)tail;
            node->d3 = (intptr_t)host->hostId;
            dzCot = AllocDzCot( host, SS_FIRST );
            dzCot->priority = CP_FIRST;
            SetCotEntry( dzCot, LazyFreeMemEntry, (intptr_t)node );
            SendRmtCot( host, i, FALSE, dzCot );
        }
    }
    StopLazyTimer( host );
}

void __stdcall RmtHostFirstEntry( intptr_t context )
{
    int i;
    DzHost* host;
    DzRmtCotFifo* fifo;
    DzSysParam* param;
    
    host = GetHost();
    fifo = NULL;
    for( i = 0; i < host->hostCount; i++ ){
        if( host->servMask & ( 1 << i ) ){
            if( !fifo ){
                host->checkFifo = host->rmtFifoArr + i;
                fifo = host->checkFifo;
            }else{
                fifo->next = host->rmtFifoArr + i;
                fifo = fifo->next;
            }
            fifo->rmtCotArr = (DzCot**)
                AllocChunk( host, sizeof( DzCot* ) * RMT_CALL_FIFO_SIZE );
            *fifo->readPos = 1;
            *fifo->writePos = 1;
            fifo->rmtCotArr[0] = NULL;
        }else{
            *host->rmtFifoArr[i].readPos = 0;
            *host->rmtFifoArr[i].writePos = 0;
            host->rmtFifoArr[i].rmtCotArr = NULL;
        }
    }
    if( fifo ){
        fifo->next = host->checkFifo;
    }

    param = (DzSysParam*)context;
    param->result = DS_OK;
    fifo = host->mgr->hostArr[0]->rmtFifoArr + host->hostId;
    fifo->rmtCotArr[0] = param->hs.returnCot;
    NotifyRmtFifo( host->mgr, host->mgr->hostArr[0], fifo->writePos, 0 );
}

void __stdcall RunRmtHostMain( intptr_t context )
{
    int ret;
    DzSysParam* param;
    DzRmtCotFifo* fifo;

    param = (DzSysParam*)context;
    ret = RunHost(
        param->hs.hostMgr, param->hs.hostId, param->hs.lowestPri,
        param->hs.dftPri, param->hs.dftSSize, RmtHostFirstEntry, context, NULL
        );
    if( ret != DS_OK ){
        param->result = ret;
        fifo = param->hs.hostMgr->hostArr[0]->rmtFifoArr + param->hs.hostId;
        fifo->rmtCotArr[0] = param->hs.returnCot;
        NotifyRmtFifo( param->hs.hostMgr, param->hs.hostMgr->hostArr[0], fifo->writePos, 0 );
    }
}

void __stdcall StartRmtHostRetEntry( intptr_t context )
{
    DzHost* host = GetHost();
    DzSysParam* param = (DzSysParam*)context;

    SetEvt( host, param->hs.evt );
}

void __stdcall MainHostFirstEntry( intptr_t context )
{
    int i;
    DzHost* host;
    DzSysParam* cotParam;
    DzSysParam param[ DZ_MAX_HOST ];
    DzCot* tmpCotArr[ DZ_MAX_HOST ];
    DzSynObj* evt;
    DzRmtCotFifo* fifo;
    DzCot* dzCot;

    host = GetHost();
    host->checkFifo = host->rmtFifoArr;
    for( i = 0; i < host->hostCount; i++ ){
        host->rmtFifoArr[i].rmtCotArr = tmpCotArr + i;
        host->rmtFifoArr[i].next = host->rmtFifoArr + i + 1;
    }
    host->rmtFifoArr[ i - 1 ].next = host->rmtFifoArr;
    evt = CreateCdEvt( host, host->hostCount - 1 );
    cotParam = (DzSysParam*)context;
    for( i = 1; i < host->hostCount; i++ ){
        dzCot = AllocDzCot( host, SS_FIRST );
        dzCot->priority = CP_FIRST;
        SetCotEntry( dzCot, StartRmtHostRetEntry, (intptr_t)&param[i] );
        param[i].threadEntry = RunRmtHostMain;
        param[i].hs.evt = evt;
        param[i].hs.hostMgr = host->mgr;
        param[i].hs.returnCot = dzCot;
        param[i].hs.hostId = i;
        param[i].hs.lowestPri = host->lowestPri;
        param[i].hs.dftPri = host->dftPri;
        param[i].hs.dftSSize = host->dftSSize;
        StartSystemThread( param + i, THREAD_STACK_MIN );
    }
    WaitSynObj( host, evt, -1 );
    CloseSynObj( host, evt );
    for( i = 1; i < host->hostCount; i++ ){
        if( param[i].result != DS_OK ){
            cotParam->result = param[i].result;
            return;
        }
    }
    fifo = NULL;
    for( i = 0; i < host->hostCount; i++ ){
        if( host->servMask & ( 1 << i ) ){
            if( !fifo ){
                host->checkFifo = host->rmtFifoArr + i;
                fifo = host->checkFifo;
            }else{
                fifo->next = host->rmtFifoArr + i;
                fifo = fifo->next;
            }
            fifo->rmtCotArr = (DzCot**)
                AllocChunk( host, sizeof( DzCot* ) * RMT_CALL_FIFO_SIZE );
            *fifo->readPos = 1;
            *fifo->writePos = 1;
            fifo->rmtCotArr[0] = NULL;
        }else{
            *host->rmtFifoArr[i].readPos = 0;
            *host->rmtFifoArr[i].writePos = 0;
            host->rmtFifoArr[i].rmtCotArr = NULL;
        }
    }
    if( fifo ){
        fifo->next = host->checkFifo;
    }
    cotParam->result = StartCot(
        host, cotParam->cs.entry, cotParam->cs.context,
        host->dftPri, host->dftSSize
        );
}

void __stdcall WaitFifoWritableEntry( intptr_t context )
{
    int rmtId;
    DzCot* dzCot;
    DzLItr* lItr;
    DzHost* host;

    host = GetHost();
    host->scheduleCd++;
    rmtId = (int)context;
    if( rmtId == host->hostId ){
        host->rmtFifoArr[ rmtId ].rmtCotArr[0] = NULL;
        InitSList( host->pendRmtCot + rmtId );
        NotifySysAutoEvt( host->mgr->sysAutoEvt + rmtId );
        return;
    }
    MoveCurCotToRmt( host, rmtId, -1 );
    host->rmtFifoArr[ rmtId ].rmtCotArr[0] = NULL;
    rmtId = host->hostId;
    host = GetHost();
    host->scheduleCd++;
    lItr = GetChainAndResetSList( &host->pendRmtCot[ rmtId ] );
    dzCot = MEMBER_BASE( lItr, DzCot, lItr );
    dzCot->priority -= CP_DEFAULT + 1;
    SendRmtCot( host, rmtId, TRUE, dzCot );
}

DzCot* CreateWaitFifoCot( DzHost* host )
{
    DzCot* ret;

    ret = AllocDzCot( host, SS_FIRST );
    ret->priority = CP_FIRST;
    SetCotEntry( ret, WaitFifoWritableEntry, (intptr_t)host->hostId );
    return ret;
}

void __stdcall WorkerMain( intptr_t context )
{
    DzHostsMgr* hostMgr;
    DzSysParam* param;
    DzWorker worker;
    DzWorker volatile* wk;
    DzHost* host;
    DzCot* dzCot;
    int rmtId;
    int nowDepth;
    BOOL eventInit;

    eventInit = FALSE;
    param = (DzSysParam*)context;
    hostMgr = param->wk.hostMgr;
    worker.entry = param->wk.entry;
    worker.context = param->wk.context;
    worker.dzCot = param->wk.dzCot;
    wk = &worker;
    while( 1 ){
        dzCot = wk->dzCot;
        if( !wk->dzCot ){
            break;
        }
        rmtId = dzCot->hostId;
        wk->entry( wk->context );
        host = hostMgr->hostArr[ rmtId ];
        do{
            WaitSysAutoEvt( hostMgr->sysAutoEvt + rmtId );
        }while( SendRmtCot( host, rmtId, FALSE, dzCot ) == FALSE );
        NotifySysAutoEvt( hostMgr->sysAutoEvt + rmtId );

        nowDepth = AtomDecInt( &hostMgr->workerNowDepth );
        if( nowDepth <= 0 ){
            nowDepth = AtomIncInt( &hostMgr->workerNowDepth );
            break;
        }else{
            if( !eventInit ){
                eventInit = TRUE;
                InitSysAutoEvt( &worker.sysEvt );
            }
            AtomPushSList( &hostMgr->workerPool, &worker.lItr );
            WaitSysAutoEvt( &worker.sysEvt );
            wk = &worker;
        }
    }
    if( eventInit ){
        FreeSysAutoEvt( &worker.sysEvt );
    }
}